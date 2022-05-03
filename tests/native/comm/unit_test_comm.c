#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "comm_protocol.h"
#include "comm_parser.h"
#include "socket.h"
#include "logger.h"

#include "crc_linux.h"

extern int _rrq_phandler(void *priv, uint8_t *data, size_t size);
extern int _wrq_phandler(void *priv, uint8_t *data, size_t size);
extern int _data_phandler(void *priv, uint8_t *data, size_t size);
extern int _err_phandler(void *priv, uint8_t *data, size_t size);

#define VERY_LONG_DRIVER_NAME "ThisIsAVeryLongDriverNameWhichIsUsedToTest" \
	"ForPossibleOverflowsOnFunctionsLikeStrcmpStrcpEtc...This Should" \
	"be long enough by now"

const static uint8_t fake_rrq_cmd[] = { 0x00, COMMP_RRQ,
					0x00, 0x00, 0x01, 0x00, // 4 byte length in bytes to read
					0x61, 0x70, 0x70, 0x72, 0x6f, 0x6d,	0x30, 0x00 // "approm0\0"
					};

const static uint8_t fake_rrq_cmd_spi0[] = { 0x00, COMMP_RRQ,
					0x00, 0x00, 0x01, 0x00, // 4 byte length in bytes to read
					0x73, 0x70, 0x69, 0x30, 0x00 // "spi0\0"
					};


const static uint8_t fake_wrq_cmd[] = { 0x00, COMMP_WRQ,
					0x61, 0x70, 0x70, 0x72, 0x6f, 0x6d,	 0x31, 0x00 // "approm1\0"
					};

const static uint8_t fake_data_cmd[COMMP_PACKET_SIZE] = { 0x00, COMMP_DATA,
							  0x00,
							  0x00 };

const static uint8_t fake_err_cmd[COMMP_ERROR_BUFFER_SIZE] = { 0x00, COMMP_ERR,
							       0x00,
							       COMMP_ERR_SEQ };

static struct comm_ctxt_t fake_tranfer_ctxt = {
	.transfer_in_progress	= false,
	.last_blocknr		= -1,
};

struct bootloader_ctxt_t _bctxt;

void phandler_should_fail_tests(void **states)
{
	LOG_RAW("");
	LOG_INFO("Running RRQ Tests");
	/* Invalid buffer */
	int error = _rrq_phandler(NULL, NULL, -1);
	assert_int_equal(error, -1);

	/* Invalid length to small */
	error = _rrq_phandler(NULL, (uint8_t *)fake_rrq_cmd, -1);
	assert_int_equal(error, -1);

	/* Invalid length to big */
	error = _rrq_phandler(NULL, (uint8_t *)fake_rrq_cmd, 1025);
	assert_int_equal(error, -1);

	/* Invalid packet. WRQ instead of RRQ */
	error = _rrq_phandler(NULL, (uint8_t *)fake_wrq_cmd, sizeof(fake_rrq_cmd));
	assert_int_equal(error, -1);

	LOG_INFO("Running WRQ Tests");
	/* Invalid context */
	error = _wrq_phandler(NULL, NULL, -1);
	assert_int_equal(error, -1);

	/* Invalid buffer */
	error = _wrq_phandler(&fake_tranfer_ctxt, NULL, -1);
	assert_int_equal(error, -1);

	/* Invalid length to small */
	error = _wrq_phandler(&fake_tranfer_ctxt, (uint8_t *)fake_wrq_cmd, -1);
	assert_int_equal(error, -1);

	/* Invalid length to big */
	error = _wrq_phandler(&fake_tranfer_ctxt, (uint8_t *)fake_wrq_cmd,
			      1025);
	assert_int_equal(error, -1);

	/* Invalid packet. WRQ instead of RRQ */
	error = _wrq_phandler(&fake_tranfer_ctxt, (uint8_t *)fake_rrq_cmd,
			      sizeof(fake_wrq_cmd));
	assert_int_equal(error, -1);

	/* Tranfer already in progress */
	struct comm_ctxt_t comm_copy = fake_tranfer_ctxt;
	comm_copy.transfer_in_progress = true;

	error = _wrq_phandler(&comm_copy, (uint8_t *)fake_rrq_cmd,
			      sizeof(fake_wrq_cmd));
	assert_int_equal(error, -1);

	LOG_INFO("Running DATA Tests");
	/* Invalid context */
	error = _data_phandler(NULL, NULL, -1);
	assert_int_equal(error, -1);

	/* Invalid buffer */
	error = _data_phandler(&fake_tranfer_ctxt, NULL, -1);
	assert_int_equal(error, -1);

	/* Invalid length to small */
	error = _data_phandler(&fake_tranfer_ctxt, (uint8_t *)fake_data_cmd,
			       -1);
	assert_int_equal(error, -1);

	/* Invalid length to big */
	error = _data_phandler(&fake_tranfer_ctxt, (uint8_t *)fake_data_cmd,
			       1025);
	assert_int_equal(error, -1);

	/* Invalid packet. WRQ instead of data */
	error = _data_phandler(&comm_copy, (uint8_t *)fake_wrq_cmd,
			       sizeof(fake_wrq_cmd));
	assert_int_equal(error, -1);

	/* Invalid packet, wrong sequence number */
	/* Last blocknr = 1020, received blocknumber 0 */
	comm_copy.last_blocknr = 1020;
	error = _data_phandler(&comm_copy, (uint8_t *)fake_data_cmd,
			       sizeof(fake_data_cmd));
	assert_int_equal(error, -1);
}

void phandler_should_pass_tests(void **states)
{
	LOG_RAW("");
	LOG_INFO("RRQ phandler should pass - approm0");
	int error = _rrq_phandler(&fake_tranfer_ctxt, (uint8_t *)fake_rrq_cmd,
				  sizeof(fake_rrq_cmd));

	assert_return_code(error, 0);

	LOG_INFO("RRQ phandler should pass - spi0");
	error = _rrq_phandler(&fake_tranfer_ctxt, (uint8_t *)fake_rrq_cmd_spi0,
				  sizeof(fake_rrq_cmd_spi0));

	assert_return_code(error, 0);

	LOG_INFO("WRQ phandler should pass");
	error = _wrq_phandler(&fake_tranfer_ctxt, (uint8_t *)fake_wrq_cmd,
			      sizeof(fake_wrq_cmd));
	assert_return_code(error, 0);

	struct comm_ctxt_t comm_copy = fake_tranfer_ctxt;

	comm_copy.transfer_in_progress = true;
	comm_copy.last_blocknr = -1;

	LOG_INFO("Data phandler should pass");
	error = _data_phandler(&comm_copy, (uint8_t *)fake_data_cmd,
			       sizeof(fake_data_cmd));
	assert_return_code(error, 0);
}

void find_driver_name_should_fail_tests(void **state)
{
	LOG_RAW("");
	LOG_INFO("Testing driver name tests (should fail)");

	LOG_INFO("Testing NULL ptr driver name");
	struct comm_driver_t *drv = comm_protocol_find_driver(NULL);

	assert_null(drv);

	LOG_INFO("Testing invalid driver name");
	drv = comm_protocol_find_driver("invalid_driver_name");
	assert_null(drv);

	LOG_INFO("Testing very long driver name");
	drv = comm_protocol_find_driver(VERY_LONG_DRIVER_NAME);
	assert_null(drv);

	LOG_INFO("Testing name thats close to existing name");
	drv = comm_protocol_find_driver("unit "); //!< "unit" is a valid name
	assert_null(drv);

	LOG_OK("Driver name tests passed (should fail)");
	return;
}

void commp_stack_should_fail_tests(void **state)
{
	LOG_RAW("");
	LOG_INFO("Testing COMMP Stack (should fail)");

	int error = comm_protocol_init();

	assert_return_code(error, 0);

	LOG_INFO("Testing comm_protocol_run with invalid driver");

	error = comm_protocol_run(&_bctxt, NULL, NULL, NULL);
	assert_int_equal(error, -1);

	LOG_INFO("Testing comm_protocol_run with invalid packet");
	struct comm_driver_t *drv = comm_protocol_find_driver("unit");

	assert_non_null(drv);

	error = comm_protocol_init();
	assert_return_code(error, 0);

	/* Zero empty packet */
	will_return(__wrapper_unit_read, 0x0);
	error = comm_protocol_run(&_bctxt, drv, NULL, NULL);
	assert_int_equal(error, -1);

	comm_protocol_close();
	LOG_OK("Driver name tests passed (should fail)");
}

void commp_stack_out_of_order_should_fail_tests(void **state)
{
	LOG_RAW("");
	LOG_INFO("Testing comm_protocol_run with invalid packet");
	struct comm_driver_t *drv = comm_protocol_find_driver("unit");

	assert_non_null(drv);

	int error = comm_protocol_init();

	assert_return_code(error, 0);

	/* No tranfer in progress data packet*/
	will_return(__wrapper_unit_read, COMMP_DATA);
	will_return(__wrapper_unit_read, true);
	will_return(__wrapper_unit_read, 0); //!< Packet number
	error = comm_protocol_run(&_bctxt, drv, NULL, NULL);
	assert_int_equal(error, -1);

	/* No tranfer in progress data packet*/
	will_return(__wrapper_unit_read, COMMP_ACK);
	will_return(__wrapper_unit_read, 0); //!< Packet number
	error = comm_protocol_run(&_bctxt, drv, NULL, NULL);
	assert_int_equal(error, -1);

	/* Second WRQ.. */
	will_return(__wrapper_unit_read, COMMP_WRQ);
	will_return(__wrapper_unit_read, COMMP_ROMNAME_ROM0); //!< Packet number

	will_return(__wrapper_unit_read, COMMP_WRQ);
	will_return(__wrapper_unit_read, COMMP_ROMNAME_ROM0); //!< Packet number
	error = comm_protocol_run(&_bctxt, drv, NULL, NULL);
	assert_int_equal(error, -1);

	/* RRQ after WRQ .. */
	will_return(__wrapper_unit_read, COMMP_WRQ);
	will_return(__wrapper_unit_read, COMMP_ROMNAME_ROM0); //!< Packet number

	will_return(__wrapper_unit_read, COMMP_RRQ);
	will_return(__wrapper_unit_read, COMMP_ROMNAME_ROM0); //!< Packet number
	error = comm_protocol_run(&_bctxt, drv, NULL, NULL);
	assert_int_equal(error, -1);

	comm_protocol_close();
	LOG_OK("COMMP Out of order test (should fail)");
}

void find_driver_name_should_pass_tests(void **state)
{
	LOG_RAW("");
	LOG_INFO("Testing driver name tests (should pass)");

	LOG_INFO("Testing valid driver name");
	struct comm_driver_t *drv = comm_protocol_find_driver("unit");

	assert_non_null(drv);

	LOG_OK("Driver name tests passed (should pass)");
}

void commp_stack_should_pass_tests(void **state)
{
	LOG_RAW("");
	LOG_INFO("Testing COMMP Stack (should pass)");

	struct comm_driver_t *drv = comm_protocol_find_driver("unit");
	assert_non_null(drv);

	LOG_INFO("Test comm_protocol_init");

	int error = 0;
	error = comm_protocol_init();
	assert_return_code(error, 0);

	will_return(__wrapper_unit_read, COMMP_WRQ);
	will_return(__wrapper_unit_read, "approm0");

	will_return(__wrapper_unit_read, COMMP_CMD);
	will_return(__wrapper_unit_read, COMMP_CMD_END);
	error = comm_protocol_run(&_bctxt, drv, NULL, NULL);
	assert_return_code(error, 0);

	will_return(__wrapper_unit_read, COMMP_WRQ);
	will_return(__wrapper_unit_read, "approm0");

	/* Disable logger to reduce spam (and time) */
	int tmp_loglvl = logger_get_loglvl();
	logger_set_loglvl(LOG_LVL_ERROR);


	for (int i = 1; i < 100; i++) {
		will_return(__wrapper_unit_read, COMMP_DATA);
		will_return(__wrapper_unit_read, true);
		will_return(__wrapper_unit_read, i);
	}

	will_return(__wrapper_unit_read, COMMP_CMD);
	will_return(__wrapper_unit_read, COMMP_CMD_CRC);

	/* Run a second time. CRC Should have ended previous
	 * trasfer */
	will_return(__wrapper_unit_read, COMMP_WRQ);
	will_return(__wrapper_unit_read, "approm0");

	for (int i = 1; i < 100; i++) {
		will_return(__wrapper_unit_read, COMMP_DATA);
		will_return(__wrapper_unit_read, true);
		will_return(__wrapper_unit_read, i);
	}

	will_return(__wrapper_unit_read, COMMP_CMD);
	will_return(__wrapper_unit_read, COMMP_CMD_CRC);

	will_return(__wrapper_unit_read, COMMP_CMD);
	will_return(__wrapper_unit_read, COMMP_CMD_END);
	error = comm_protocol_run(&_bctxt, drv, NULL, NULL);
	assert_return_code(error, 0);

	/* Restore log level */
	logger_set_loglvl(tmp_loglvl);

	comm_protocol_close();

	LOG_OK("Stack tests passed (should pass)");
}

void commp_stack_long_term_stress_test()
{
	LOG_RAW("");
	LOG_INFO("Testing COMMP Stack (should pass)");

	struct comm_driver_t *drv = comm_protocol_find_driver("unit");
	assert_non_null(drv);

	LOG_INFO("Test comm_protocol_init");

	int error = 0;
	error = comm_protocol_init();
	assert_return_code(error, 0);

	will_return(__wrapper_unit_read, COMMP_WRQ);
	will_return(__wrapper_unit_read, "approm0");

	/* Disable logger to reduce spam (and time) */
	int tmp_loglvl = logger_get_loglvl();
	logger_set_loglvl(LOG_LVL_ERROR);

	bool first_run = true;
	uint16_t blocknr = 0;

	/* Queue roughly 3.3M Packets. Should be enough to stress test the parser.*/
	for (int runs = 0; runs < 25; runs++) {
		for (uint16_t i = 0; i < (uint16_t)-1; i++) {
			if (first_run) {
				blocknr++; //!< Increment i on first run. 0 is reserved.
				first_run = false;
			}
			will_return(__wrapper_unit_read, COMMP_DATA);
			will_return(__wrapper_unit_read, true);
			will_return(__wrapper_unit_read, blocknr);
			blocknr++;
		}

		for (uint16_t i = 0; i < (uint16_t)-1; i++) {
			will_return(__wrapper_unit_read, COMMP_DATA);
			will_return(__wrapper_unit_read, true);
			will_return(__wrapper_unit_read, blocknr);
			blocknr++;
		}
	}

	will_return(__wrapper_unit_read, COMMP_DATA);
	will_return(__wrapper_unit_read, false);
	will_return(__wrapper_unit_read, blocknr);

	/* Send a CRC */
	will_return(__wrapper_unit_read, COMMP_CMD);
	will_return(__wrapper_unit_read, COMMP_CMD_CRC);

	/* End the stack */
	will_return(__wrapper_unit_read, COMMP_CMD);
	will_return(__wrapper_unit_read, COMMP_CMD_END);

	error = comm_protocol_run(&_bctxt, drv, NULL, NULL);
	assert_return_code(error, 0);

	/* Restore log level */
	logger_set_loglvl(tmp_loglvl);

	comm_protocol_close();

	LOG_OK("Stress tests passed (should pass)");
}

void commp_parser_fuzzing_test_should_fail(void **state)
{
	LOG_RAW("");
	LOG_INFO("Testing fuzzing (should fail)");

	struct comm_driver_t *drv = comm_protocol_find_driver("unit");
	assert_non_null(drv);

	LOG_INFO("Test comm_protocol_init");

	int error = 0;
	error = comm_protocol_init();
	assert_return_code(error, 0);

	uint8_t dummy_buffer[COMMP_PACKET_SIZE]; //!< Don't init, nice rubbish data

	/* Disable logger to reduce spam (and time) */
	int tmp_loglvl = logger_get_loglvl();
	logger_set_loglvl(LOG_LVL_DEBUG);

	for (uint16_t i = 0; i < COMMP_NR_OF_OPCODES; i++) {
		dummy_buffer[COMMP_OPCODE_BYTE] = (uint8_t)i;
		dummy_buffer[COMMP_CMD_CMDCODE_MSB] = 0x4f;     //!< Force command byte
		dummy_buffer[COMMP_CMD_CMDCODE_LSB] = 0xf5;     //!< Force command byte
		error = comm_protocol_parse_packet(&fake_tranfer_ctxt,
						   dummy_buffer,
						   COMMP_PACKET_SIZE);
	  if (error != -1)
			LOG_ERROR("%d \t Expected -1 for error = %d", i, error);
		assert_int_equal(error, -1);
	}

	/* Tranfer already in progress */
	struct comm_ctxt_t comm_copy = fake_tranfer_ctxt;
	comm_copy.transfer_in_progress = true;

	for (uint16_t i = 0; i < COMMP_NR_OF_OPCODES; i++) {
		dummy_buffer[COMMP_OPCODE_BYTE] = (uint8_t)i;
		error = comm_protocol_parse_packet(&comm_copy, dummy_buffer,
						   COMMP_PACKET_SIZE);
	  if (error != -1)
			LOG_ERROR("%d \t Expected -1 for error = %d", i, error);
		assert_int_equal(error, -1);
	}

	/* Restore log level */
	logger_set_loglvl(tmp_loglvl);

	comm_protocol_close();
}

int main(void)
{
	cmocka_set_message_output(CM_OUTPUT_XML);

	int failed_test = 0;

	const struct CMUnitTest tests_that_should_fail[] = {
		cmocka_unit_test(phandler_should_fail_tests),
		cmocka_unit_test(find_driver_name_should_fail_tests),
		cmocka_unit_test(commp_stack_should_fail_tests),
		cmocka_unit_test(commp_stack_out_of_order_should_fail_tests),
		cmocka_unit_test(commp_parser_fuzzing_test_should_fail),
	};

	const struct CMUnitTest tests_that_should_pass[] = {
		cmocka_unit_test(phandler_should_pass_tests),
		cmocka_unit_test(find_driver_name_should_pass_tests),
		cmocka_unit_test(commp_stack_should_pass_tests),
		cmocka_unit_test(commp_stack_long_term_stress_test),
	};

	failed_test += cmocka_run_group_tests(tests_that_should_fail, NULL, NULL);
	failed_test += cmocka_run_group_tests(tests_that_should_pass, NULL, NULL);

	return failed_test;
}
