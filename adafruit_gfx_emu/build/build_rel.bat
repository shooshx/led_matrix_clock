rem --profiling -s DEMANGLE_SUPPORT=1
rem TBD BUG this doesn't work without -g3
%EMSCRIPTEN%\emcc --pre-js ..\pre_js_print.js -s DEMANGLE_SUPPORT=1 -s ALLOW_MEMORY_GROWTH=0 -s WASM=0 -O3 -g3 -s TOTAL_MEMORY=20054016 --memory-init-file 0 -DHAVE_SNPRINTF -DHAVE_VSNPRINTF -DHAVE_STRLCPY -DHAVE_STRNLEN -DOF_NO_OBJ -DOF_NO_ELF32 -DOF_NO_ELF64 -DOF_NO_ELFX32 -DOF_NO_AOUT -DOF_NO_AOUTB -DOF_NO_COFF -DOF_NO_WIN32 -DOF_NO_WIN64 -DOF_NO_AS86 -DOF_NO_RDF2 -DOF_NO_IEEE -DOF_NO_MACHO32 -DOF_NO_MACHO64 -DOF_NO_DBG   --memory-init-file 0 -Wno-switch -I. -Ioutput -Ix86 -Idisasm -Iasm -Iinclude   asm\assemble.c asm\directbl.c asm\directiv.c asm\error.c asm\eval.c asm\exprdump.c asm\exprlib.c asm\float.c asm\labels.c asm\listing.c asm\nasm.c asm\parser.c asm\pptok.c asm\pragma.c asm\preproc-nop.c asm\preproc.c asm\quote.c asm\rdstrnum.c asm\segalloc.c asm\stdscan.c asm\strfunc.c asm\tokhash.c common\common.c  macros\macros.c nasmlib\badenum.c nasmlib\bsi.c nasmlib\crc64.c nasmlib\file.c nasmlib\filename.c nasmlib\hashtbl.c nasmlib\ilog2.c nasmlib\malloc.c nasmlib\md5c.c nasmlib\mmap.c nasmlib\path.c nasmlib\perfhash.c nasmlib\raa.c nasmlib\rbtree.c nasmlib\readnum.c nasmlib\realpath.c nasmlib\saa.c nasmlib\srcfile.c nasmlib\string.c nasmlib\strlist.c nasmlib\ver.c nasmlib\zerobuf.c output\codeview.c output\legacy.c output\nulldbg.c output\nullout.c output\outbin.c output\outform.c output\outlib.c stdlib\snprintf.c stdlib\strlcpy.c stdlib\strnlen.c stdlib\vsnprintf.c x86\disp8.c x86\iflag.c x86\insnsa.c x86\insnsb.c x86\insnsd.c x86\insnsn.c x86\regdis.c x86\regflags.c x86\regs.c x86\regvals.c -s EXPORTED_FUNCTIONS="['_run_nasm']" -o ../../war/asm_js_main.html  -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap']"

rem output\outcoff.c output\outdbg.c   output\outieee.c  output\outmacho.c output\outobj.c output\outrdf2.c output\outaout.c
rem disasm\disasm.c disasm\sync.c