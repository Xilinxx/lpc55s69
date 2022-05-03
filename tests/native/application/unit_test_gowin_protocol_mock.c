/*********************** (C) COPYRIGHT BARCO  *********************************
* File Name           : unit_test_comm_protocol_mock.c  - native
* Author              : DAVTH
* created             : 14/06/2021
* Description         : protocol mock - redefinition/wrapping

* History:
* 14/6/2021 - initial
*******************************************************************************/
#include <string.h>
#include <stdio.h>

#include "logger.h"

#include "comm/gowin_protocol.h"

extern t_uart_data_raw UART_DATA[2];

//------------------------------------------------------------------------------
void __wrap_GOWIN_TX_SendMsg (t_gowin_command cmd)
{
  GOWIN_DATA.WaitForReply = true;
  if ((cmd == ACK) | (cmd == NACK)) // single byte reply
  {
    GOWIN_DATA.WaitForReply = false;
  }

  if(cmd == READ_EDID)
  {
    GOWIN_Protocol_state_set(GOWIN_WAIT_FOR_EDID);
    LOG_DEBUG("UNIT_TEST - GOWIN_WAIT_FOR_EDID");
  }
  else if(cmd == READ_DPCD)
  {
    GOWIN_Protocol_state_set(GOWIN_WAIT_FOR_DPCD);
    LOG_DEBUG("UNIT_TEST - GOWIN_WAIT_FOR_DPCD");
  }
  else if (cmd == WRITE_EDID)
  {
    LOG_DEBUG("UNIT_TEST - GOWIN_WAIT_FOR_EDID_WRITE_ACK");
    GOWIN_Protocol_state_set(GOWIN_WAIT_FOR_EDID_WRITE_ACK);
  }
  else if (cmd == WRITE_DPCD) // 256byte of data is sent, reply with ACK
  {
    LOG_DEBUG("UNIT_TEST - GOWIN_WAIT_FOR_EDID_WRITE_ACK");
    GOWIN_Protocol_state_set(GOWIN_WAIT_FOR_EDID_WRITE_ACK);
  }
  else if (cmd == READ_ETH_STATUS)
  {
    LOG_DEBUG("UNIT_TEST - %s - GOWIN_READING_1BYTE",returnCmdName(cmd));
    GOWIN_Protocol_state_set(GOWIN_READING_1BYTE);
    StartByte();
  }
  else if (cmd == READ_BL_CURRENT)
  {
    LOG_DEBUG("UNIT_TEST - %s - GOWIN_READING_2BYTE",returnCmdName(cmd));
    GOWIN_Protocol_state_set(GOWIN_READING_2BYTE);
    StartByte();
  }
  else {
    LOG_DEBUG("UNIT_TEST - GOWIN %s '%c' sent",returnCmdName(cmd), cmd);
    GOWIN_Protocol_state_set(GOWIN_WAIT_FOR_ACK);
  }
}

