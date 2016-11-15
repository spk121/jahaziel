---------=---------+---------+---------+---------=---------+---------+---------+
ISO/IEC 9281 rev 1 (1990) is a standard that describes a method of coding
pictorial or audio information as part of character stream.  By itself,
it really is more of a skeleton of a standard, and not a useful standalone
standard.

It defines a bunch of nomenclature and acronyms.

PE: Picture Entity -- a complete data message
PCE: Picture Control Entity -- a header
PDE: Picture Data Entity -- a payload
PCD: Picture Coding Delimiter -- start of header
CMI: Picture Method Identifier -- message type
LI: Length Indicator -- length of payload

```
+----------------------+
|          PE          |
+----------------+-----+
|       PCE      | PDE |
+-----+-----+----+-----+
| PCD | CMI | LI | PDE |
+-----+-----+----+-----+
```

PCD starts the header. It is 0x1B 0x70

CMI identified the type of data. It is two bytes.
Byte 1 is the Picture Mode and is between 0x20 and 0x3E.
Byte 2 is an Picture Identifier and is between 0x40 and 0x7F.

Then the LI + PDE has one of two scenarios:

1. LI gives no length indication, and the PDE is terminated by a special character.
2. LI gives a number of bytes and the PDE has that exact number of bytes.

In case 1, LI is 0x00. It is followed immediately by the PDE. The PDE ends with a special
terminating character.  The terminating character
is not defined in this spec, but is related to the CMI.

In case 2, The 1st byte of LI is 0xFF.
Then one or more bytes decode to a single unsigned integer length.
The length is followed by the PDE, which has exactly that specified length.
