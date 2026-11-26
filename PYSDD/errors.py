
class SDDError(Exception):
    pass


class SDDSyntaxError(SDDError):
    pass


class SDDTypeError(SDDError):
    pass


class SDDDuplicateKeyError(SDDError):
    pass


class SDDVersionError(SDDError):
    pass

class SDDFileError(SDDError):
    pass