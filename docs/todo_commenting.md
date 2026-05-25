# TODO commenting

All TODO style comments follow the convention of `// [type]: [severity], [subsystem] - [short explanation]`

`type` can be of either "TODO" or "HACK".
`severity` can be either:

- benign - used for things that are very unlikely to affect anything else
- minor - used for things that will affect something else, or are mildly incorrect
- major - used for things that will affect something else, and their impact is substantial
- global - used for things that'll affect the kernel as a whole, and their impact is substantial

## TODO comments

TODO comments should be used for things that, whilst the general implementation is correct, require some work, or additional features.

## HACK comments

HACK comments should be used for things that are incorrect/used as a workaround.
