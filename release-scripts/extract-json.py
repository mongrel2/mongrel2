#!/bin/env python
import json
from sys import stdin, argv, stdout
obj = json.load(stdin)
for item in argv[1:]:
    obj=obj[item]
stdout.write(str(obj)+'\n')
