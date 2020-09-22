```metadata
tags: postgres, pg type, number
```

### float precision problem
Many languages have following similar problems (I've tested with js and python).

``` python
>>> 0.2 + 0.7 + 0.1
0.9999999999999999
>>> 0.3 + 0.6
0.8999999999999999
```

The reason is that computer world is composed by 0 and 1. We can easily convert 
integer to binary format. However, most decimals can't. 0.5 can write as `2^-1`. 
But how about 0.2, 0.3, 0.7 ? They can't be write as finite binary. So that you 
cannot get exact value of them and of course the calculation of them are also 
not exact.

To avoid this problem, some language have `decimal` module to do decimal calculation.
For postgres, you can use `numeric` type to achieve same result.

### NaN of numeric
Take care about `'NaN'` of numeric type. Any operation on `NaN` yields `NaN`. So 
`sum` or `avg` on numeric type may get `NaN` if any row has `NaN`.

Moreover, postgres treats `NaN` as equal and greater than all other values. So `max` 
will get `NaN` if one row has it.

### Infinity, NaN of float
`Infinity, -Infinity, NaN` are defined in IEEE 754. And they are valid in postgres. 
However, IEEE 754 specifies that `NaN` should not compare equal to any other including
 itself while postgres treats `NaN` as equal and greater than all other including 
 `Infinity`.

### serial and sequence
`serial, smallserial, bigserial` are not real types (you can check if from `pg_type` 
table). They are just syntax helper to achieve `AUTO_INCREMENT` property.

If you set a column `serial`, it will create a new sequence automatically and set 
this column `NOT NULL DEFAULT nextval('xxxx_seq')`.

Attention, set a column `serial` do not mean this column is primary key by default. And 
it do not mean this column is unique. If you want unique or primary key, you need to 
set it by yourself.

### references
[official doc: datatype-numeric](https://www.postgresql.org/docs/10/static/datatype-numeric.html)
