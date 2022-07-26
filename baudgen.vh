//-----------------------------------------------------------------------------
//-- Constantes para el modulo de generacion de baudios para comunicaciones serie
//------------------------------------------------------------------------------
//-- (C) BQ. September 2015. Written by Juan Gonzalez (Obijuan)
//------------------------------------------------------------------------------

//-- Para la icestick el calculo es el siguiente:
//-- Divisor = 100 000 000 / BAUDIOS  (Y se redondea a numero entero)

//-- Valores de los divisores para conseguir estos BAUDIOS:

`define B800000 125
`define B500000 200
`define B400000 250
`define B200000 500
`define B115200 868
`define B100000 1000
`define B57600  1736
`define B38400  2604

`define B19200  5208
`define B9600   10417
`define B4800   20833
`define B2400   41667
`define B1200   83333
`define B600    166667
`define B300    333333





