import PYSDD
from decimal import Decimal

players = PYSDD.load("PYSDD/tests/players.sdd")

print(players["values"]["players"][1]["level"])