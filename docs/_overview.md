# The Jahaziel Terminal Widget

Jahaziel is a receiving device and renderer for a coded character data elements.
It receives coded character data elements and renders it to a screen.
It is coded in a mix of C# and Ragel.

From ECMA-6 
> A receiving device shall be capable of receiving within a CC-data-element and
> interpreting the coded representations of graphic characters from one or more
> graphic character sets, and an identified selection of control functions and
> code-identification functions conforming to this Standard.

> Such a device shall make available to the user, from an appropriate set, characters
> or other indications which are implicitly or explicitly determined by the graphic
> characters, control functions, and code-identification functions whose coded
> representations are received.

Jahaziel is born out of my interest in obsolete specifications, as such, it will
try to correctly render as many obsolete and near-obsolete specifications as
entertains me.  Notably, it will try to render some of the Videotex extensions,
including the audio and photographic extensions.

## TODO

First off, will be rendering ECMA 6, aka ISO 646, which is related to ASCII.
It starts of as a standard ASCII, but with the following complications
- It allows for overstruck characters. Backspace and Carriage Return can be
  used to make overstruck characters.
- It has escape sequences that allow one to change the C0 and G0 character
  sets

## Specification Tower
- [ ] ECMA-6 aka ISO 646:  "7-Bit Coded Character Set"
- [ ] ECMA-35 aka ISO 2022:  "Character Code Structure and Extension Techniques"
- [ ] ECMA-48: "Control Functions for Coded Character Sets"
- [ ] ISO 9281: "Information technology - Picture coding method"
- [ ] ETS 300 072: "Terminal Equipment (TE); Videotex Presentation Layer protocol, Videotex presentation layer data syntax".
- [ ] ETS 300 073: "Videotex presentation layer data syntax, Geometric Display (CEPT Recommendation T/TE 06-02, Edinburgh 1988)".
- [ ] ETS 300 074: "Videotex presentation layer data syntax transparent data (CEPT Recommendation T/TE 06-03, Edinburgh 1988)".
- [ ] ETS 300 075: "Terminal Equipment (TE); Videotex processable data".
- [ ] ETS 300 076: "Terminal Equipment (TE); Videotex, Terminal Facility Identifier (TFI)".
- [ ] ETS 300 177: "Terminal Equipment (TE); Videotex, Photographic syntax".
- [ ] ETS 300 149: "Terminal Equipment (TE); Videotex: Audio syntax"
- [ ] CCITT Recommendation G.711 (1988): "Pulse code modulation of voice frequencies".
- [ ] CCITT Recommendation G.721 (1988): "32 kbit/s adaptive differential pulse code modulation.
- [ ] CCITT Recommendation G.723 (1988): "Extensions of Recommendation G.721 ADPCM to 24 and 40 kbit/s for DCME application".
- [ ] CCITT Recommendation G.722 (1988): "7 kHz audio-coding within 64 kbit/s".
- [ ] GSM Specification 06.10: "GSM Full-rate speech transcoding".
- [ ] CCITT Recommendation J.41: "Characteristics of equipment for the coding of analogue high quality sound programme signals for transmission on 384 kbit/s channels".
- [ ] CCITT Recommendation J.42: "Characteristics of equipment for the coding of analogue medium quality sound programme signals for transmission on 384 kbit/s channels".
- [ ] CCITT Recommendation H.261 (1988): "Common intermediate format".
- [ ] CCITT Recommendation T.51: "Latin based coded character sets for telematic services".
- [ ] CCITT Recommendation T.61 (1988): "Character repertoire and coded character sets for the international teletex service".
- [ ] CCIR Recommendation 601-1 (1986): "Encoding Parameters of Digital Television For Studios".
