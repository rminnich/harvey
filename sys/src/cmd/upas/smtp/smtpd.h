enum {
	ACCEPT = 0,
	REFUSED,
	DENIED,
	DIALUP,
	BLOCKED,
	DELAY,
	TRUSTED,
	NONE,

	MAXREJECTS = 100,
};

	
typedef struct Link Link;
typedef struct List List;

struct Link {
	Link *next;
	String *p;
};

struct List {
	Link *first;
	Link *last;
};

extern	int	fflag;
extern	int	rflag;
extern	int	sflag;

extern	int	debug;
extern	NetConnInfo	*nci;
extern	char	*dom;
extern	char*	me;
extern	List	ourdoms;
extern	ulong	peerip;
extern	int	trusted;

void	auth(String *, String *);
int	blocked(String*);
int	cidrcheck(char*);
void	data(void);
char*	dumpfile(char*);
int	forwarding(String*);
void	getconf(void);
void	hello(String*, int extended);
void	help(String *);
void	listadd(List*, String*);
void	listfree(List*);
int	masquerade(String*, char*);
void	noop(void);
int	optoutofspamfilter(char*);
void	quit(void);
void	parseinit(void);
void	receiver(String*);
int	recipok(char*);
int	reply(char*, ...);
void	reset(void);
int	rmtdns(char*, char*);
void	sayhi(void);
void	sender(String*);
void	starttls(void);
void	turn(void);
void	verify(String*);
int	zzparse(void);
