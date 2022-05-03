# Zeus 300S Application Unit Test

* [Intro](#intro)
* [Structure](#Structure)
* [Software](#Software)

## Intro

* [CMOCKA - webpage](https://cmocka.org/)
* [Testing info GitLab](https://gitlab.kitware.com/cmake/community/-/wikis/doc/ctest/Testing-With-CTest)
* [Mocking with CMOCKA](https://satish.net.in/20160403/)

## Structure

| Directory | What's in here?|
|---|---|
| CMakeList.txt | cmake compiler file |
| unit_test_comm_protocol.c | Communication Protocol test (byte/integer) |
| unit_test_comm_protocol_helper.c | Communication Protocol helper function |
| unit_test_comm_protocol_mock.c | Communication Protocol wrapped functions |
| unit_test_comm_protocol_array.c | Communication Protocol test (array) |
| unit_test_comm_protocol_bootcode.c | Communication Protocol test (bootcode) |
| unit_test_comm_gowin_protocol.c | Gowin Protocol test |
| unit_test_comm_gowin_protocol_mock.c | Gowin Mocked Protocol test |
|---|---|


___
## Software
### Compile on server
Code
```sh
./s300-scripts/build.sh -t arm
```

build and run tests
```sh
./s300-scripts/test.sh -t cmocka
```
Below additional description for using the `-i` interactive mode.<br>
___
### Compile inside docker container
```sh
make unit_comm_gowin_protocol_test
```



Run all tests (alias is 'make test' )
```sh
ctest
```

Run a specific test
```sh
ctest -R unit_lcd_protocol_test

Test project /home/dev/build
    Start 3: unit_lcd_protocol_test
1/1 Test #3: unit_lcd_protocol_test ...........   Passed    0.00 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.01 sec
```
or with verbose on,
```sh
dev@a0c8140d6837:~/build$ ctest -R unit_comm_protocol_test -V
UpdateCTestConfiguration  from :/home/dev/build/DartConfiguration.tcl
UpdateCTestConfiguration  from :/home/dev/build/DartConfiguration.tcl
Test project /home/dev/build
Constructing a list of tests
Done constructing a list of tests
Updating test list for fixtures
Added 0 tests to meet fixture requirements
Checking test dependency graph...
Checking test dependency graph end
test 2
    Start 2: unit_comm_protocol_test

2: Test command: /home/dev/build/src/application/test/native/unit_comm_protocol_test
2: Test timeout computed to be: 10000000
2: [==========] Running 1 test(s).
2: [ RUN      ] int_rw_testregister_msg_test
2: [DEBUG] (          data_map.c)(                 init_data_map @ 94) : Application Version [APP-07:35_21-09-08_v0.1.]
2: [DEBUG] (          data_map.c)(                 init_data_map @ 95) : Boot Version [07:35_21-09-08_v0.1.1-14]
2: [ INFO] (unit_test_comm_protocol.c)(  int_rw_testregister_msg_test @185) : Read/write 4 byte testregister - should pass
2: [DEBUG] (          protocol.c)(                 COMM_Protocol @130) : COMM_START_BYTE
2: [DEBUG] (          protocol.c)(                 COMM_Protocol @134) : COMM_STOP_BYTE
2: [DEBUG] (          protocol.c)(                 COMM_Protocol @148) : COMM_RETURN_NEW_MSG
2: [DEBUG] (              comm.c)(                  comm_handler @140) : comm_handler - Address match
2: [DEBUG] (          comm_run.c)(               Run_CommHandler @ 73) : Run_CommHandler
2: [DEBUG] (          comm_run.c)(                   c4ByteWrite @285) : c4ByteWrite - 0xaa test register write 0x1020304
2: [DEBUG] (unit_test_comm_protocol_mock.c)(       __wrap_PROTO_TX_SendMsg @ 72) : PROTO_TX_SendMsg { FE, 10, 15, 01, 26, FF }
2: [ INFO] (unit_test_comm_protocol.c)(  int_rw_testregister_msg_test @194) : 4 byte test register is 01020304
2: [DEBUG] (          protocol.c)(                 COMM_Protocol @130) : COMM_START_BYTE
2: [DEBUG] (          protocol.c)(                 COMM_Protocol @134) : COMM_STOP_BYTE
2: [DEBUG] (          protocol.c)(                 COMM_Protocol @148) : COMM_RETURN_NEW_MSG
2: [DEBUG] (              comm.c)(                  comm_handler @140) : comm_handler - Address match
2: [DEBUG] (          comm_run.c)(               Run_CommHandler @ 73) : Run_CommHandler
2: [DEBUG] (          comm_run.c)(                    c4ByteRead @197) : Read4Byte - testregister
2: [DEBUG] (unit_test_comm_protocol_mock.c)(       __wrap_PROTO_TX_SendMsg @ 72) : PROTO_TX_SendMsg { FE, 10, 14, 01, 02, 03, 04, 2E, FF }
2: [       OK ] int_rw_testregister_msg_test
2: [==========] 1 test(s) run.
2: [  PASSED  ] 1 test(s).
1/1 Test #2: unit_comm_protocol_test ..........   Passed    0.00 sec

The following tests passed:
        unit_comm_protocol_test

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.01 sec
```

Exclude a specific test
```sh
ctest -E unit_comm_test
Test project /home/dev/build
    Start 1: unit_gowin_protocol_test
1/2 Test #1: unit_gowin_protocol_test .........   Passed    0.00 sec
    Start 2: unit_lcd_protocol_test
2/2 Test #2: unit_lcd_protocol_test ...........   Passed    0.00 sec

100% tests passed, 0 tests failed out of 2

Total Test time (real) =   0.01 sec
```

Run the executable or enable verbose flag
```sh
./src/application/test/native/unit_gowin_protocol_test

or

ctest -R unit_gowin_protocol_test -V
```
___

### Communication Protocol unit tests
 * gowin_protocol  -  The initial command communication protocol
 * comm_protocol   -  Communication protocol betw Zynq & gpmcu
    * protocol = byte , 4byte messages
    * array-messages
    * bootcode
 * mock/wrap functions

*Mocking remark:*  The wrapping of sharedram_log_read with `-Wl,--wrap=sharedram_log_read` was not succesful. Using `-Wl,--defsym,sharedram_log_read=__wrap_sharedram_log_read` does work as expected.


___
## Remarks
___
## To do
 * [ *NO GO* ] Gowin Protocol to be replaced by comm_protocol in FPGA code.
