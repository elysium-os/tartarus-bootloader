#!/bin/sh

PREFIX="/usr/local"
ENVIRONMENT="development"
VENDOR_EFI_REQUIRED="no"

while [[ $# -gt 0 ]]; do
    case $1 in
        --prefix=*)
            PREFIX=${1#*=}
            ;;
        --platform=*)
            PLATFORM=${1#*=}
            case $PLATFORM in
                x86_64-bios)
                    ;;
                x86_64-uefi)
                    VENDOR_EFI_REQUIRED="yes"
                    ;;
                *)
                    echo "Unknown platform \"$PLATFORM\""
                    exit 1
                    ;;
            esac
            ;;
        --production)
            ENVIRONMENT="production"
            ;;
        --toolchain-compiler=*)
            TC_COMPILER=${1#*=}
            ;;
        --toolchain-linker=*)
            TC_LINKER=${1#*=}
            ;;
        --toolchain-objcopy=*)
            TC_OBJCOPY=${1#*=}
            ;;
        --vendor-libgcc=*)
            VENDOR_LIBGCC=${1#*=}
            ;;
        --vendor-efi=*)
            VENDOR_EFI=${1#*=}
            ;;
        -*|--*)
            echo "Unknown option \"$1\""
            exit 1
            ;;
        *)
            POSITIONAL_ARGS+=("$1")
            ;;
    esac
    shift
done
set -- "${POSITIONAL_ARGS[@]}"

if [ -z "$PLATFORM" ]; then
    >&2 echo "No platform provided"
    exit 1
fi

if [ -z "$TC_COMPILER" ]; then
    >&2 echo "No compiler provided"
    exit 1
fi

if [ -z "$TC_LINKER" ]; then
    >&2 echo "No linker provided"
    exit 1
fi

if [ -z "$TC_OBJCOPY" ]; then
    >&2 echo "No objcopy provided"
    exit 1
fi

SRC_DIRECTORY=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
DEST_DIRECTORY=$(pwd)

cp $SRC_DIRECTORY/Makefile $DEST_DIRECTORY

CONFIG_FILE="$DEST_DIRECTORY/conf.mk"
echo "export SRC_DIRECTORY := $SRC_DIRECTORY" > $CONFIG_FILE
echo "export BUILD_DIRECTORY := $DEST_DIRECTORY/build" >> $CONFIG_FILE
echo "export PREFIX := $PREFIX" >> $CONFIG_FILE
echo "export PLATFORM := $PLATFORM" >> $CONFIG_FILE
echo "export ENVIRONMENT := $ENVIRONMENT" >> $CONFIG_FILE

echo "export CC := $TC_COMPILER" >> $CONFIG_FILE
echo "export OBJCOPY := $TC_OBJCOPY" >> $CONFIG_FILE
echo "export LD := $TC_LINKER" >> $CONFIG_FILE
echo "export NASM := nasm" >> $CONFIG_FILE

if [ -z "$VENDOR_LIBGCC" ]; then
    >&2 echo "LIBGCC dependency missing"
    exit 1
fi
echo "export VENDOR_LIBGCC := $VENDOR_LIBGCC" >> $CONFIG_FILE

if [ $VENDOR_EFI_REQUIRED = "yes" ]; then
    if [ -z "$VENDOR_EFI" ]; then
        >&2 echo "EFI dependency missing"
        exit 1
    fi
    echo "export VENDOR_EFI := $VENDOR_EFI" >> $CONFIG_FILE
fi