# Uart 

```
START  DATA (LSB → MSB)   STOP
 0      1 0 0 0 0 0 1 0    1

```


```
0100000101 0100001001 0100001101
```
Each group = one UART frame.


```
Tick starts → line goes '0'
Tick duration → 104 µs (for 9600 baud)
Line stays '0' the whole tick
Next tick → line might change for next bit
```


## TODO

write receiver
```

```