/* Variety of Winbond and compatible chips */

enum winbond_chips {
	NOSENSER,
	W83781D,
	W83782D,
	W83783S,
	W83791D,
	W83627HF,
	W83697HF,
	WBUNKNOWN,
	AS99127F,
	ASB100,
	ASM58,
	LM78,
	LM79,
	ADM9240,
	UNKNOWN
};

#ifndef NO_WINBCHIP

static char *winbchip[] = {
	"No Sensor",
	"Winbond Chip W83781D",
	"Winbond Chip W83782D",
	"Winbond Chip W83783S",
	"Winbond Chip W83791D",
	"Winbond Chip W83627HF",
	"Winbond Chip W83697HF",
	"Winbond unknown Chip",
	"Asus Chip AS99127F",
	"Asus Chip ASB100(Bach)",
	"Asus Chip ASM58(Mozart-2)",
	"Nat.Semi.Con. Chip LM78",
	"Nat.Semi.Con. Chip LM79",
	"Analog Dev. Chip ADM9240",
	"Unknown Chip assuming LM78",
	NULL };

#endif
