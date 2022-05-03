#!/bin/sh
# the spiFlashBitfile.sh is a test program for Flashing the (Gowin) Zeus SPI Flash
#
# Example commands
#   i2ctransfer -y 1 w2@0x60 0x50 0x9f #Bulk Erase
#   i2ctransfer -y 1 w2@0x60 0x50 0xd8 x x #Block Erase 64k


# initialize options variables
READ_MODE=-1
BUS=1
CHIP_ADDR=0x60
CHIP_FLASH_CMD=0x50
CHIP_FLASH_CMD_PP=0x02 #Page Program
CHIP_FLASH_CMD_64KERASE=0xd8
START_ADDR=0x00
WRITE_OFFSET=0
MAX_BYTES_CNT=256
FILENAME="GW_GreenPower_top.bin"

# function to check decimal format
#  returns: 0 if format is correct, otherwise 1
check_dec_format() {
  if echo $1 | grep -q -E "^[0-9]+$"; then return 0; fi
  return 1
}

# function to check hex format
#  returns: 0 if format is correct, otherwise 1
check_hex_format() {
  if echo $1 | grep -q -E "^0x[0-9a-fA-F]+$"; then return 0; fi
  return 1
}

# process options
while [ $# -ge 1 ]; do
  case "$1" in
    -b|--bus)
      shift
      BUS="$1"
      ;;
    -c|--chip_addr)
      shift
      CHIP_ADDR=$1
      ;;
    -l|--length)
      shift
      MAX_BYTES_CNT=$1
      ;;
    -o|--offset)
      shift
      WRITE_OFFSET=$1
      ;;
    -s|--start_addr)
      shift
      START_ADDR=$1
      ;;
    -f|--file)
      shift
      FILENAME="$1"
      ;;
    *)
      echo "ERR: unrecognized mode option '$1'"
      print_help 1
      ;;
  esac
  shift
done

# check if bus is used
if [ -z "$BUS" ]; then
  echo "ERR: i2c bus has to be specified"
  exit 3
fi

# check if i2c bus is available
if ! i2cdetect -l | grep -q "i2c-$BUS"$(printf '\t'); then
  echo "ERR: i2c bus 'i2c-$BUS' was not found"
  exit 3
fi

# check if chip address is in hex format
if ! check_hex_format $CHIP_ADDR; then
  echo "ERR: chip address '$CHIP_ADDR' has to be in hex format"
  exit 5
fi



FILESIZE=$(wc -c "$FILENAME" | awk '{print $1}')
BLOCKS=$((FILESIZE / 256)) # Amount of Pages to program
REMAINDER=$((FILESIZE % 256)) # PageProgram 256bytes
ERASEBLOCKS=$((FILESIZE / 65536 + 1)) #The amount of 64k blocks to erase


echo "Erasing SPI Flash in Blocks of 64k"
while [ "$ERASEBLOCKS" -gt 0 ]
do
  SECTOR_HI_BYTE=`printf '0x%X' "$((ERASE_ADR>>8))"`
  SECTOR_LO_BYTE=`printf '0x%X' "$((ERASE_ADR & 0xFF))"`
  set -x
  i2ctransfer -y "$BUS" w4@$CHIP_ADDR $CHIP_FLASH_CMD $CHIP_FLASH_CMD_64KERASE $SECTOR_HI_BYTE $SECTOR_LO_BYTE
  set +x
  ERASEBLOCKS=`expr $ERASEBLOCKS - 1`
  ERASE_ADR=$(( $ERASE_ADR + 0x100 ))
done

sleep 1

export COUNTER=0
while true
do
  #Start with writing the PageWrite command with StartBlock
  OFFSET=$((COUNTER * MAX_BYTES_CNT))
  COUNTER_LO_BYTE=`printf '0x%X' "$(( COUNTER & 0xFF ))"`
  COUNTER_HI_BYTE=`printf '0x%X' "$((COUNTER>>8))"` 
  i2ctransfer -y "$BUS" w4@$CHIP_ADDR $CHIP_FLASH_CMD $CHIP_FLASH_CMD_PP $COUNTER_HI_BYTE $COUNTER_LO_BYTE
  #Fetch the 256 bytes from the bin-file
  BYTES=$(xxd -l $MAX_BYTES_CNT -c 1 -p "$FILENAME" -s $OFFSET | awk '{printf "0x%s ",$0}')
  chrlen=${#BYTES} # normally 1280 chars(256*5)
  if [ "$chrlen" -lt 1280 ]
  then
    echo "...Add 0xFF dummy values to last block"
    while [ `echo $BYTES | awk '{print length}'` -le 1275 ]
    do
      BYTES=$(printf "%s 0xff" "$BYTES")
    done
  fi
  i2ctransfer -y "$BUS" w$MAX_BYTES_CNT@$CHIP_ADDR $BYTES
  COUNTER=`expr $COUNTER + 1`
  if [ "$COUNTER" -gt "$BLOCKS" ]
  then
    break
  fi
  echo -ne "Writing $COUNTER/$BLOCKS \033[0K\r"   
done
echo "flashed!"