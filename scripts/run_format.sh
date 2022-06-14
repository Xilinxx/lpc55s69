#!/bin/bash

uncrustify -c ./uncrustify_c.cfg --no-backup --replace  `find .  -name "*.c" -or -name "*.h" -not -path "./vendor/**"`
