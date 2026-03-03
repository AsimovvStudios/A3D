### if statemnts
```c
/* like this */
if (x)
    a();
else
    b();

if (x) {
    a();
    b();
}
else {
    c();
    d();
}
```

### comments
```c
/* like this */
// not this

/* they must be simple and
   follow the style of the
   rest of the project.
   NO capitals, bad grammar,
   easy to read. rarely use
   "the" and other bloat words
*/
```

### functions
```c
/* each function must have a prototype 
   be it in the header or source file.
   (headers for public functions, source
   for static ones)
*/

/* prototypes and implementations MUST
   be in alphabetical order!
*/

/* static function have priority over
   normal ones
*/

/* if no parameters, use void! */

static int a(void);
static int b(void);
static int c(void);
static int d(void);

int aa(void);
int bb(void);
int cc(void);
int dd(void);
/* ^^^ this is in source header */

static int a(void)
{
    ...
}

static int b(void)
{
    ...
}

...

int aa(void)
{
    ...
}

int bb(void)
{
    ...
}
```

### indents
all indents are 1 tab wide  

all structs delclerations must
have at least a 16 char gap between
the type and variable name
```c
struct x {
    int [...] dx;
    int [...] dy;
};
```

### too long line
if a function is too long,
format it like this
```c
function_too_long_with_too_many_params(
    p1, p2, p3, p4, p5, p6,
    b1, b2, b3, b4, b5, b6,
)
```

