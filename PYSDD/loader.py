#
#   file: SDD/PYSDD/loader.py
#   
#   Creator: Aleksander Sapieha (sourceguy68@gmail.com)
#
#   Contributors:
#   Aleksander Sapieha (sourceguy68@gmail.com)
#

from PYSDD.parser import Parser
from PYSDD.errors import SDDFileError

import pickle

def loads(source):
    """Parse SDD source text."""
    return Parser(source).parse()


def load(filepath):
    """Load and parse an SDD file."""
    with open(filepath, "r", encoding="utf-8") as file:
        if not file.name.lower().endswith(".sdd"):
            raise SDDFileError("Please use a .sdd file extension")
        source = file.read()

    return loads(source)
