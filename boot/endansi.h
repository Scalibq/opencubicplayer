// changed version, be sure to save line 130 before re-saving in AcidDraw. thanks.

static const int endansi_WIDTH =80;
static const int endansi_DEPTH = 15;
static const int endansi_LENGTH = 2400;
static unsigned char endansi [] = {
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    '�',0x08,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    '�',0x0B,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,'�',0x08,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,'O',0x03,'p',0x03,
    'e',0x03,'n',0x03,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    '�',0x08,' ',0x08,' ',0x08,' ',0x08,' ',0x08,'�',0x08,' ',0x08,
    '�',0x08,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    '�',0x0B,'�',0x0B,'�',0x3B,'�',0x03,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,'�',0x0B,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,'�',0x08,' ',0x08,' ',0x08,
    '�',0x08,' ',0x08,' ',0x08,'�',0x08,' ',0x08,'�',0x08,'�',0x08,
    '�',0x08,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x08,
    '�',0x08,' ',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,
    '�',0x08,'�',0x08,'�',0x08,'�',0x08,' ',0x08,'�',0x03,'�',0x03,
    '�',0x03,'�',0x3B,' ',0x3B,'�',0x3B,'�',0x0B,'�',0x0B,' ',0x0B,
    '�',0x08,' ',0x08,'�',0x08,'�',0x08,'�',0x08,' ',0x08,'�',0x08,
    ' ',0x08,'�',0x0B,'�',0x3B,' ',0x07,' ',0x07,'�',0x0B,' ',0x0B,
    '�',0x0B,' ',0x0B,'�',0x03,'�',0x39,'�',0x39,'�',0x03,' ',0x03,
    '�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,' ',0x08,'�',0x0B,
    ' ',0x0B,'�',0x08,'�',0x08,'�',0x08,' ',0x08,'�',0x08,' ',0x08,
    '�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,
    '�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,
    '�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,' ',0x08,
    '�',0x08,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,'�',0x08,'�',0x08,'�',0x08,' ',0x08,'�',0x03,
    '�',0x03,'�',0x39,'�',0x39,' ',0x39,'�',0x39,'�',0x03,'�',0x03,
    '�',0x03,'�',0x3B,'�',0x0B,' ',0x0B,'�',0x0B,'�',0x0B,'�',0x0B,
    '�',0x07,' ',0x07,' ',0x07,'�',0x39,'�',0x39,' ',0x07,' ',0x07,
    ' ',0x07,'�',0x03,' ',0x03,' ',0x03,' ',0x03,'�',0x03,'�',0x03,
    ' ',0x33,'�',0x03,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,'�',0x08,' ',0x08,'�',0x3B,'�',0x0B,'�',0x0B,'�',0x0B,
    '�',0x0B,' ',0x0B,' ',0x0B,'�',0x0B,'�',0x3B,'�',0x3B,'�',0x3B,
    '�',0x03,'�',0x03,' ',0x03,'.',0x08,'t',0x07,'m',0x07,'.',0x08,
    ' ',0x08,'�',0x08,'�',0x08,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,'�',0x08,
    ' ',0x08,'�',0x03,'�',0x39,'�',0x03,'�',0x39,'�',0x03,'�',0x03,
    ' ',0x03,' ',0x03,'�',0x08,' ',0x08,'�',0x03,'�',0x03,'�',0x3B,
    '�',0x3B,'�',0x3B,'�',0x03,' ',0x03,' ',0x03,'�',0x03,'�',0x39,
    '�',0x03,' ',0x03,'�',0x39,'�',0x39,'�',0x03,' ',0x03,'�',0x08,
    '�',0x08,' ',0x08,'�',0x03,'�',0x39,' ',0x07,' ',0x07,'�',0x08,
    '�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x03,
    '�',0x3B,'�',0x03,'�',0x03,' ',0x03,' ',0x03,'�',0x03,'�',0x03,
    '�',0x03,'�',0x08,' ',0x08,' ',0x08,'�',0x39,'�',0x03,'�',0x03,
    ' ',0x03,' ',0x03,' ',0x03,'�',0x08,'�',0x08,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,'�',0x39,'�',0x03,'�',0x39,'�',0x03,
    ' ',0x03,' ',0x03,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,
    '�',0x08,' ',0x08,'�',0x03,'�',0x03,'�',0x03,' ',0x03,' ',0x03,
    '�',0x03,'�',0x39,'�',0x03,' ',0x03,'�',0x08,'�',0x03,'�',0x03,
    '�',0x39,'�',0x03,' ',0x03,'�',0x08,' ',0x08,'�',0x03,'�',0x39,
    ' ',0x07,'�',0x03,'�',0x03,'�',0x03,'�',0x03,'�',0x03,' ',0x03,
    ' ',0x03,'�',0x08,' ',0x08,'�',0x3B,'�',0x39,'�',0x03,' ',0x03,
    '�',0x03,'�',0x03,'�',0x03,'�',0x08,'�',0x08,'�',0x08,'�',0x03,
    '�',0x03,'�',0x03,' ',0x03,' ',0x03,'�',0x03,' ',0x03,' ',0x03,
    '�',0x08,' ',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,'�',0x08,' ',0x08,'�',0x09,
    '�',0x39,'�',0x39,'�',0x03,' ',0x03,'�',0x08,'�',0x08,'�',0x08,
    '�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,' ',0x08,
    '�',0x08,' ',0x08,'�',0x09,'�',0x39,'�',0x39,'�',0x39,'�',0x03,
    ' ',0x03,' ',0x03,'�',0x39,'�',0x39,'�',0x09,' ',0x09,'�',0x08,
    ' ',0x08,'�',0x03,'�',0x39,' ',0x07,' ',0x07,'�',0x03,'�',0x39,
    '�',0x39,'�',0x03,'�',0x03,'�',0x09,'�',0x03,' ',0x03,'�',0x03,
    '�',0x39,'�',0x03,' ',0x03,'�',0x03,'�',0x03,' ',0x03,'�',0x08,
    '�',0x08,'�',0x08,'�',0x08,'�',0x08,' ',0x08,'�',0x03,'�',0x03,
    ' ',0x03,'�',0x08,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    '�',0x08,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,'�',0x09,'�',0x39,' ',0x07,' ',0x07,
    ' ',0x07,'�',0x08,' ',0x08,' ',0x08,'�',0x09,'�',0x09,'�',0x01,
    '�',0x09,'�',0x09,'�',0x09,' ',0x09,'�',0x09,'�',0x09,'�',0x09,
    '�',0x39,'�',0x09,'�',0x09,'�',0x39,'�',0x39,'�',0x09,'�',0x09,
    '�',0x19,'�',0x09,' ',0x09,' ',0x09,'�',0x09,'�',0x39,'�',0x09,
    '�',0x08,'�',0x08,'�',0x08,' ',0x08,' ',0x08,' ',0x08,'�',0x09,
    '�',0x09,' ',0x09,'�',0x09,'�',0x39,'�',0x39,'�',0x03,'�',0x03,
    ' ',0x03,'�',0x08,'�',0x08,'�',0x08,'�',0x08,' ',0x08,'�',0x03,
    '�',0x03,'�',0x39,'�',0x03,'�',0x03,' ',0x03,' ',0x03,'�',0x08,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,'�',0x08,' ',0x08,' ',0x08,' ',0x08,' ',0x08,
    '�',0x09,'�',0x09,'�',0x09,'�',0x09,'�',0x09,'�',0x09,'�',0x09,
    '�',0x09,'�',0x09,'�',0x09,'�',0x01,'�',0x09,'�',0x39,'�',0x09,
    '�',0x09,'�',0x09,' ',0x09,'�',0x09,'�',0x09,' ',0x09,' ',0x09,
    ' ',0x09,' ',0x09,'�',0x09,'�',0x19,'�',0x01,' ',0x01,'�',0x19,
    '�',0x19,'�',0x09,'�',0x09,'�',0x09,'�',0x09,'�',0x09,'�',0x09,
    '�',0x19,'�',0x19,'�',0x19,' ',0x07,' ',0x07,'�',0x19,'�',0x09,
    '�',0x09,'�',0x09,' ',0x09,'�',0x09,'�',0x09,'�',0x09,'�',0x19,
    '�',0x19,'�',0x09,'�',0x39,'�',0x39,'�',0x39,'�',0x03,'�',0x03,
    '�',0x03,'�',0x03,' ',0x03,'�',0x03,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    '�',0x08,' ',0x08,'�',0x08,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,'�',0x09,' ',0x09,' ',0x09,'D',0x08,'B',0x08,'!',0x08,
    ' ',0x08,' ',0x08,' ',0x08,'�',0x09,'�',0x09,'�',0x19,'�',0x19,
    '�',0x01,'�',0x01,' ',0x01,'�',0x01,' ',0x01,'�',0x08,' ',0x08,
    ' ',0x08,'�',0x08,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,'�',0x01,'�',0x01,'�',0x01,' ',0x01,' ',0x01,' ',0x01,
    '�',0x09,'�',0x09,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,'�',0x01,'�',0x01,'�',0x01,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,'�',0x08,'�',0x08,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x08,' ',0x08,' ',0x08,'�',0x08,' ',0x08,
    '�',0x08,'�',0x08,'�',0x08,' ',0x08,'�',0x08,'�',0x08,'�',0x08,
    ' ',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,
    '�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,
    '�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,
    '�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,
    '�',0x08,'�',0x08,'.',0x08,'-',0x08,' ',0x08,'P',0x09,' ',0x09,
    'L',0x09,' ',0x09,'A',0x09,' ',0x09,'Y',0x09,' ',0x09,'E',0x09,
    ' ',0x09,'R',0x09,' ',0x09,'-',0x08,'.',0x08,'-',0x08,' ',0x08,
    VERSION[0],0x09,VERSION[1],0x09,VERSION[2],0x09,VERSION[3],0x09, VERSION[4],0x09,VERSION[5],0x09,(VERSION[6]?'�':' '),0x09,
    '-',0x08,'.',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,'�',0x08,
    ' ',0x08,'�',0x08,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,'h',0x0B,'t',0x0B,
    't',0x0B,'p',0x0B,':',0x0F,'/',0x0F,'/',0x0F,'w',0x0B,'w',0x0B,
    'w',0x0B,'.',0x0F,'c',0x0B,'u',0x0B,'b',0x0B,'i',0x0B,'c',0x0B,
    '.',0x0F,'o',0x0B,'r',0x0B,'g',0x0B,'/',0x0F,'p',0x0B,'l',0x0B,
    'a',0x0B,'y',0x0B,'e',0x0B,'r',0x0B,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x08,' ',0x07,'d',0x08,'o',0x08,'n',0x08,
    '\'',0x08,'t',0x08,' ',0x08,'m',0x08,'i',0x08,'s',0x08,'s',0x08,
    ' ',0x08,'t',0x08,'h',0x08,'e',0x08,' ',0x08,'b',0x08,'e',0x08,
    's',0x08,'t',0x08,' ',0x08,'6',0x08,'4',0x08,'K',0x08,' ',0x08,
    'i',0x08,'n',0x08,'t',0x08,'r',0x08,'o',0x08,' ',0x08,'e',0x08,
    'v',0x08,'e',0x08,'r',0x08,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    'f',0x03,'t',0x03,'p',0x03,':',0x0B,'/',0x0B,'/',0x0B,'f',0x03,
    't',0x03,'p',0x03,'.',0x0B,'c',0x03,'u',0x03,'b',0x03,'i',0x03,
    'c',0x03,'.',0x0B,'o',0x03,'r',0x03,'g',0x03,'/',0x0B,'p',0x03,
    'u',0x03,'b',0x03,'/',0x0B,'p',0x03,'l',0x03,'a',0x03,'y',0x03,
    'e',0x03,'r',0x03,' ',0x07,' ',0x07,' ',0x07,'�',0x08,' ',0x07,
    ' ',0x08,' ',0x08,' ',0x08,' ',0x08,' ',0x08,'h',0x07,'t',0x07,
    't',0x07,'p',0x07,':',0x0F,'/',0x0F,'/',0x0F,'w',0x07,'w',0x07,
    'w',0x07,'.',0x0F,'t',0x07,'h',0x07,'e',0x07,'p',0x07,'r',0x07,
    'o',0x07,'d',0x07,'u',0x07,'c',0x07,'t',0x07,'.',0x0F,'d',0x07,
    'e',0x07,' ',0x09,' ',0x09,' ',0x09,' ',0x09,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x08,' ',0x08,' ',0x07,
    '.',0x08,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,'o',0x09,'p',0x09,'e',0x09,'n',0x09,
    'c',0x09,'p',0x09,'@',0x03,'g',0x09,'m',0x09,'x',0x09,'.',0x03,
    'n',0x09,'e',0x09,'t',0x09,' ',0x09,' ',0x09,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x0B,'.',0x08,' ',0x0B,'a',0x08,'n',0x08,'d',0x08,' ',0x08,
    'v',0x08,'i',0x08,'s',0x08,'i',0x08,'t',0x08,' ',0x08,'t',0x08,
    'h',0x08,'e',0x08,' ',0x08,'n',0x08,'e',0x08,'x',0x08,'t',0x08,
    ' ',0x08,'M',0x08,'e',0x08,'k',0x08,'k',0x08,'a',0x08,'^',0x08,
    'S',0x08,'y',0x08,'m',0x08,'p',0x08,'o',0x08,'s',0x08,'i',0x08,
    'u',0x08,'m',0x08,' ',0x08,'.',0x08,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,'�',0x08,'�',0x08,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,'�',0x08,'�',0x08,'�',0x08,' ',0x07,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x0B,' ',0x0B,' ',0x0B,' ',0x0B,
    ' ',0x0B,' ',0x0B,' ',0x0B,' ',0x09,' ',0x09,' ',0x09,' ',0x09,
    ' ',0x09,' ',0x09,' ',0x09,' ',0x09,' ',0x09,' ',0x09,' ',0x09,
    ' ',0x09,' ',0x09,' ',0x09,' ',0x09,' ',0x09,' ',0x09,' ',0x09,
    ' ',0x07,' ',0x07,' ',0x07,' ',0x07,' ',0x07,'�',0x08,'�',0x08,
    ' ',0x07,' ',0x07,' ',0x07};