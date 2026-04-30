/* Below here are symbol references. These are required to emit the referenced symbols in the final binary,
 * since we compile with --gc-sections and other types of LTO. */

EXTERN(_shell_symbol_tbl)
EXTERN(__symbolRefDummy)