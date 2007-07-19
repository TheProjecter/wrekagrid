#all: pim2
#	gcc -DS32BITS -o pim2 -O2 pim2.c
#	gcc -DS32BITS -o pimwreka -O2 pimwreka.c
#64: pim2
#	gcc -DS64BITS -m64 -o pim2 -O2 pim2.c
#	gcc -DS64BITS -m64 -o pimwreka -O2 pimwreka.c

#Basile Clout, May 2007
#Fix for Alpha
INCLUDES= -I$(top_srcdir)/include

# C compiler flags 
AM_CFLAGS=-O2 
# preprocessor flags
AM_CPPFLAGS=-DS64BITS

bin_PROGRAMS = pim2 pimwreka

pim2_SOURCES = pim2.c
pimwreka_SOURCES = pimwreka.c



#bin_PROGRAMS=pim2

#pim2_CFLAGS = -DS32BITS
#pim2_SOURCES=pim2.c
