/* UNO's resources, compiled by Retro68's Rez and merged with the code
   (the Makefile passes the linked code via --copy). Replaces Retro68's
   default Retro68APPL.r so the app can carry its own SIZE, version and
   Finder icon. */
#include "Processes.r"
#include "Finder.r"
#include "Icons.r"

/* MultiFinder partition. The app itself is about 22K of code plus an 18K
   offscreen bitmap; Retro68's default asks for a whole megabyte, which a
   1MB Plus running MultiFinder can't spare. */
resource 'SIZE' (-1) {
	reserved,
	ignoreSuspendResumeEvents,
	reserved,
	cannotBackground,
	needsActivateOnFGSwitch,
	backgroundAndForeground,
	dontGetFrontClicks,
	ignoreChildDiedEvents,
	is32BitCompatible,
	notHighLevelEventAware,
	onlyLocalHLEvents,
	notStationeryAware,
	dontUseTextEditServices,
	reserved,
	reserved,
	reserved,
	384 * 1024,
	256 * 1024
};

resource 'vers' (1) {
	0x01, 0x00, release, 0x00, verUS,
	"1.0",
	"1.0, UNO for Vintage Computers"
};

/* The Finder icon: drawn by tools/mkicon.py, which prints this data. */
resource 'ICN#' (128) {
	{
		/* icon */
		$"00000000 0FFFC000 1AAAA000 35555000"
		$"2AA00000 3547FFF0 2A8FFFF8 355FFFFC"
		$"2A9FFFFC 355FFFFC 2A9FC1FC 355F007C"
		$"2A9E003C 355C001C 2A9C001C 3558000C"
		$"2A9955CC 35595D4C 2A995D4C 35595D4C"
		$"2A99D5CC 3558000C 2A9C001C 355C001C"
		$"2A9E003C 155F007C 0FDFC1FC 001FFFFC"
		$"001FFFFC 001FFFFC 000FFFF8 0007FFF0",
		/* mask */
		$"00000000 0FFFC000 1FFFE000 3FFFF000"
		$"3FFFFFF8 3FFFFFFC 3FFFFFFE 3FFFFFFE"
		$"3FFFFFFE 3FFFFFFE 3FFFFFFE 3FFFFFFE"
		$"3FFFFFFE 3FFFFFFE 3FFFFFFE 3FFFFFFE"
		$"3FFFFFFE 3FFFFFFE 3FFFFFFE 3FFFFFFE"
		$"3FFFFFFE 3FFFFFFE 3FFFFFFE 3FFFFFFE"
		$"3FFFFFFE 1FFFFFFE 0FFFFFFE 003FFFFE"
		$"003FFFFE 003FFFFE 003FFFFE 001FFFFC"
	}
};

/* Finder bundle: the creator 'UNOg' owns an application icon. The Finder
   only reads it when the file's hasBundle flag is set, which Rez does not
   do -- tools/setbundle.py sets it in the MacBinary before the floppy is
   built. */
resource 'FREF' (128) {
	'APPL', 0, ""
};

resource 'BNDL' (128) {
	'UNOg', 0,
	{
		'ICN#', { 0, 128 };
		'FREF', { 0, 128 }
	}
};

/* The signature ("owner") resource the bundle points at: by convention a
   Pascal string, here the version line. */
data 'UNOg' (0) {
	$"22" "UNO 1.0, UNO for Vintage Computers"
};
