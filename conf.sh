#!/bin/sh

PREFIX="/usr/local"
ENV="dev"

while [[ $# -gt 0 ]]; do
    case $1 in
        --prefix=*)
            PREFIX=${1#*=}
            ;;
        --target=*)
            TARGET=${1#*=}
            case $TARGET in
                x86_64-bios)
                    ;;
                x86_64-uefi64)
                    ;;
                *)
                    echo "Unknown target \"$TARGET\""
                    exit 1
                    ;;
            esac
            ;;
        --prod)
            ENV="prod"
            ;;
        -*|--*)
            echo "Unknown option \"$1\""
            exit 1
            ;;
        *)
            POSITIONAL_ARGS+=("$1") # save positional arg
            ;;
    esac
    shift
done
set -- "${POSITIONAL_ARGS[@]}"

if [ -z "$TARGET" ]; then
    >&2 echo "No target provided"
    exit 1
fi

SRCDIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
DSTDIR=$(pwd)

cp $SRCDIR/Makefile $DSTDIR

CONFMK="$DSTDIR/conf.mk"
echo "export TARGET := $TARGET" > $CONFMK
echo "export ENV := $ENV" >> $CONFMK

echo "export PREFIX := $PREFIX" >> $CONFMK

echo "export SRC := $SRCDIR" >> $CONFMK
echo "export BUILD := $DSTDIR/build" >> $CONFMK

echo "export GIT := git" >> $CONFMK
echo "export ASMC := nasm" >> $CONFMK
echo "export CC := x86_64-elf-gcc" >> $CONFMK
echo "export LD := x86_64-elf-ld" >> $CONFMK
echo "export OBJCOPY := x86_64-elf-objcopy" >> $CONFMK