---------=---------+---------+---------+---------=---------+---------+---------+

# ISO/IEC 9281-1 (1990)

ISO/IEC 9281 rev 1 (1990) is a standard that describes a method of coding
pictorial or audio information as part of character stream.  By itself,
it really is more of a skeleton of a standard, and not a useful standalone
standard.

## Direct Switching

To get into ISO/IEC 9281-1 mode from ISO 2022 mode
ESC 0x25 Fn
where Fn is 0x43, 0x44, or 0x41 for Videotex syntax I, II, or III respectively.

To get back to ISO 2022 mode from ISO/IEC 9281-1 mode
ESC 0x25 0x40

This method of entering and exiting picture coding methods from an ISO 2022
environment is called "Direct Switching".

## Picture Entities

It defines a bunch of nomenclature and acronyms.

- PE: Picture Entity -- a complete data message
- PCE: Picture Control Entity -- a header
- PDE: Picture Data Entity -- a payload
- PCD: Picture Coding Delimiter -- start of header
- CMI: Picture Method Identifier -- message type
- LI: Length Indicator -- length of payload

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

In case 1, LI is 0x00. It is followed immediately by the PDE. The PDE ends with a
special terminating character.  The terminating character is not defined in this
spec, but is related to the CMI.

In case 2, The 1st byte of LI is 0xFF.
Then one or more bytes decode to a single unsigned integer length.
The length is followed by the PDE, which has exactly that specified length.

### Encoding of LI

As mentioned above, if there is a length indication, then 1st byte of LI is 0xFF.

For the remaining bytes, the value of the high bit in the byte is irrelevant.

Each of the remaining bytes of LI has the 7th bit as one.

If the 6th bit of a byte is called the "extension flag". If it is zero, it is the last
byte in LI.  If it is one, more byte(s) of LI follow.

The remaining 5 bits are part of the length value.  The 1st byte of the length
value has the most significant bits.  The last byte of LI has the least significant
bits.
