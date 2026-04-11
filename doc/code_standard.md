# Code Standard

## General

Use smart tabs that are 4 spaces per tab.

Curly braces (`{`/`}`) must go on the same line.  No exceptions.

## Parentheses

There must be spaces between parentheses for all cases (except typecasts):
```c
// spaces between parentheses
printf( "notice how there's spaces between the parentheses\n" );

// spaces between parentheses
float radians = ( degrees * M_PI ) / 180.0f;

// NO spaces between parentheses
uint32_t radiansAsInt = (uint32_t) someInteger;
```

`if`/`else`, `while`, `switch`/`case`, and `for` loops must contain spaces between the keyword and the opening parenthesis:

```c
if ( 69 == 420 ) {
	// nice
} else {
	// boring
}

while ( 1 ) {
	// run the game
}

switch ( gameMode ) {
	case GAME_MODE_SINGLEPLAYER: {
		// singleplayer code

		break;
	}

	case GAME_MODE_MULTIPLAYER: {
		// multiplayer code

		break;
	}
}

for ( uint32_t i = 0; i < 10; i++ ) {
	// do something
}
```

## Variables

Variables are camelCase:

```c
uint32_t x someInteger = 0;
```

Floating point values must end in `.0f`:

```c
float countdownTimer = 10.0f;
```

Double values must end in `.0`:

```c
double delta = 0.0;
```

## Constants

Constants that are global must be uppercase separated by underscores:

```c
// naming convention applies for ALL #defines
#define NUM_FRAMEBUFFERS	3

enum {
	MAX_NUM_PIPELINE_STAGES	= 3
};

const uint32_t GAME_ARENA_SIZE	= 3 * 1024;
```

Local constants follow the same rules as variables.

## Functions

Functions are PascalCase.

In C codebases, functions that take no parameters **MUST** have `void` as the only parameter.

```c
void DoSomething( void );
```

If a function is part of a module or system, they must be prefixed appropriately.  For example, a function that's part of window management might be prefixed with `Win_`, whereas a function that's part of the file system might be prefixed with `FS_`.

## Structs, Enums, Typedefs, and Unions

All of these are camelCase and with an `_t`.

In C codebases `struct`s and `enum`s must be `typedef`'d like so:

```c
// struct members must be tab aligned
typedef struct gameState_t {
	uint32_t	myInt;
	float		myFloat;
	const char*	myString;
} gameState_t;
```

In C++, this is not necessary:

```cpp
// struct members must be tab aligned
struct gameState_t {
	uint32_t	myInt;
	float		myFloat;
	const char*	myString;
};
```

`enum` values must be uppercase separated by underscores.

The names of each enum value must also match the name of the enum (without the `_t` bit).

```c
// C example
typedef enum gameMode_t {
	GAME_MODE_SINGLEPLAYER	= 1,
	GAME_MODE_MULTIPLAYER
} gameMode_t;

// C++ example
enum gameMode_t {
	GAME_MODE_SINGLEPLAYER	= 1,
	GAME_MODE_MULTIPLAYER
};
```

## Classes

In C++, classes are PascalCase:

```c
// in classes you must also tab align things
class SoundSystem {
public:
			SoundSystem();
	virtual	~SoundSystem();

	void	Init();
	void	Shutdown();

private:
	bool	initialised;
};
```
