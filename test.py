#
#   file: SDD/test.py
#   
#   Creator: Aleksander Sapieha (sourceguy68@gmail.com)
#
#   Contributors:
#   Aleksander Sapieha (sourceguy68@gmail.com)
#

import PYSDD
from decimal import Decimal

players = PYSDD.load("PYSDD/tests/players.sdd")

print(players["values"]["players"][1]["level"])