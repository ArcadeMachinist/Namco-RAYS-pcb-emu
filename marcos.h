#ifndef BIT_READ
# define BIT_READ(value, bit)            ((value) &   (1UL << (bit)))
#endif
#ifndef BIT_SET
# define BIT_SET(value, bit)             ((value) |=  (1UL << (bit)))
#endif
#ifndef BIT_CLEAR
# define BIT_CLEAR(value, bit)           ((value) &= ~(1UL << (bit)))
#endif
#ifndef BIT_WRITE
# define BIT_WRITE(value, bit, bitvalue) (bitvalue ? BIT_SET(value, bit) : BIT_CLEAR(value, bit))
#endif
