/* SMBus IO for various chips */  

#ifdef DEBUG
#define SMB_DEBUG
#endif

#if !defined(__smbuses_h__)
#define	__smbuses_h__

struct smbus_io {
	int	 (*ReadB)(int, int, int);
	int	 (*ReadW)(int, int, int);
	int	 (*WriteB)(int, int, int, int);
	int	 (*WriteW)(int, int, int, int);
};

typedef struct smbus_io SMBUS_IO;

int set_smbus_io(int *, int *);
int chk_smbus_io(int, int);

#endif	/*__smbues_h__*/
