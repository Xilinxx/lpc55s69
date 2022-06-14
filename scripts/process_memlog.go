package main

import (
	"io/ioutil"
	"fmt"
	"encoding/binary"
	"log"
)

const LINE_LEN = 128

func main() {
	memBytes, err := ioutil.ReadFile("out.bin")
	if err != nil {
		log.Fatal("File not found")
	}

	for i := LINE_LEN; i < len(memBytes); i+=LINE_LEN {
		if (string(memBytes[i+4:i+LINE_LEN])[0] == 0) {
			continue
		}
		fmt.Printf("%d : %s\n", binary.BigEndian.Uint32(memBytes[i:i+4]),
			string(memBytes[i+4:i+LINE_LEN]))
	}
}
