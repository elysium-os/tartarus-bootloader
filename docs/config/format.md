# Tartarus Config File Format v2

## General

- The format is case sensitive.
- Only ASCII characters are respected, any unicode codepoints etc will be parsed as (likely broken) ASCII.
- Whitespace is ignored.
    - Whitespace means ASCII codes 0x9, 0xA, 0xD, 0x20.

## Values

A value is defined as any of the following.

- [Object](#object)
- [String](#string)
- [Integer](#integer)
- [Boolean](#boolean)
- [Array](#array)

## Object

An object consists of braces enclosing key-value pairs (entries) separated by equal signs.
The key can either be a [string](#string) or an identifier conforming to the following rules:

- Consists of `_`, `a-z`, `A-Z`, or `0-9` characters.
- The first character is not be `0-9`.

Example:

```
{
    x = "y"
    abc = 123
}
```

### Root exception

The file itself forms an object, however, without the braces enclosing the key-value list.

## String

Any characters surrounded by quotes. Any character after a `\` is escaped meaning quotes can be escaped using `\"` and to produce a backslash it must be escaped itself like so `\\`.

Example:

```
"hello"
"\"world\""
```

## Integer

A sequence of numeric characters optionally prefixed by a `-` or `+` interpreted as a decimal integer.

Example:

```
123
-5
+400
0
```

## Boolean

A literal `true` or `false`.

Example:

```
true
false
```

## Array

A list of values enclosed by brackets (`[]`). Note that no commas are expected, like the rest of the format.

Example:

```
[ "hello" "world" 5 true ]
```
