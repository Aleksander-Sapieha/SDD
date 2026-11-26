
from decimal import Decimal


def serialize_value(value, indent=0):
    prefix = " " * indent

    if isinstance(value, bool):
        return prefix + "boolean " + str(value).lower()

    if value is None:
        return prefix + "null"

    if isinstance(value, str):
        escaped = (
            value
            .replace("\\", "\\\\")
            .replace('"', '\\"')
            .replace("\n", "\\n")
            .replace("\t", "\\t")
        )
        return prefix + 'string "' + escaped + '"'

    if isinstance(value, int):
        return prefix + "integer " + str(value)

    if isinstance(value, Decimal):
        return prefix + "decimal " + str(value)

    if isinstance(value, float):
        raise TypeError(
            "SDD does not support Python floats. Use Decimal instead."
        )

    if isinstance(value, dict):
        lines = [prefix + "contains"]

        for key, item in value.items():
            if not isinstance(key, str):
                raise TypeError("SDD object keys must be strings")

            lines.append(
                " " * (indent + 4)
                + serialize_assignment(key, item, indent + 4)
            )

        lines.append(prefix + "end")
        return "\n".join(lines)

    if isinstance(value, list):
        lines = [prefix + "array"]

        for item in value:
            if isinstance(item, dict):
                lines.append(
                    " " * (indent + 4)
                    + serialize_value(item, indent + 4)
                )
            elif isinstance(item, list):
                lines.append(
                    serialize_value(item, indent + 4)
                )
            else:
                lines.append(
                    serialize_value(item, indent + 4)
                )

        lines.append(prefix + "end")
        return "\n".join(lines)

    raise TypeError(
        f"Unsupported Python type: {type(value).__name__}"
    )


def serialize_assignment(name, value, indent=0):
    prefix = " " * indent

    if isinstance(value, dict):
        return (
            prefix + name + " contains\n"
            + "\n".join(
                serialize_assignment(k, v, indent + 4)
                for k, v in value.items()
            )
            + "\n" + prefix + "end"
        )

    if isinstance(value, list):
        return prefix + name + " is " + serialize_value(value, indent).lstrip()

    return prefix + name + " is " + serialize_value(value).lstrip()


def dumps(data):
    if not isinstance(data, dict):
        raise TypeError("SDD root data must be a dictionary")

    lines = ["SDD 1.0"]

    for key, value in data.items():
        if not isinstance(key, str):
            raise TypeError("SDD object keys must be strings")

        lines.append(serialize_assignment(key, value))

    return "\n".join(lines) + "\n"


def dump(data, filepath):
    source = dumps(data)

    with open(filepath, "w", encoding="utf-8", newline="\n") as file:
        file.write(source)
