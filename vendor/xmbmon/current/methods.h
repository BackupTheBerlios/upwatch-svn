/* Winbond IO defaults address */  
#if !defined(__methods_h__)
#define	__methods_h__

#define IOP_ADDR	0x290
#define WBIO1	(IOP_ADDR + 0x05)
#define WBIO2	(IOP_ADDR + 0x06)

/* Winbond registor address for SMBus method */
#define LM_ADDR			0x5A
#define WBtemp1_ADDR	0x92
#define WBtemp2_ADDR	0x90

struct lm_methods {
	int	 (*Open)();
	void (*Close)();
	int	 (*Read)(int);
	void (*Write)(int, int);
	int	 (*ReadW)(int);
	void (*WriteW)(int, int);
	int	 (*ReadTemp1)();
	int	 (*ReadTemp2)();
};

typedef struct lm_methods LM_METHODS;

#endif	/*__methods_h__*/
