```metadata
tags: python, data, pandas
```

## pandas quick guide

`pandas` is a very nice tool for small to middle scale data manipulation. It can read
 data from excels, csvs and databases, do all kinds of converting, filter, calculation
 and then save to files or databases.

`DataFrame` is the core data container of pandas. A dataframe is like a table of database.
It has `columns` like table, `index` like PK of table. However, you can have same column
 and index in a database, thus different from database table.

### creation
You can create a database simply using nested list:

```python
>>> pd.DataFrame([[1,2,3]])          # simple one dimension df
   0  1  2
0  1  2  3

>>> pd.DataFrame([[1,2,3], [4,5,6,7,8]])  # two dimension, missing data is filled with NaN
   0  1  2    3    4
0  1  2  3  NaN  NaN
1  4  5  6  7.0  8.0

# you can specify index and columns
>>> pd.DataFrame([[1,2,3], [5,6,7,8]], columns=list('ABCD'), index=['r1', 'r2'])
    A  B  C    D
r1  1  2  3  NaN
r2  5  6  7  8.0

# create from dict, key became column
>>> pd.DataFrame({'A': 1, 'B': [2,6], 'C': [3,7]}, index=['r1', 'r2'])
    A  B  C    D
r1  1  2  3  NaN
r2  1  6  7  NaN

# df can have same column or index
>>> pd.DataFrame([[1,2,3], [5,6,7,8]], columns=list('ABBC'), index=['r1', 'r1'])
    A  B  B    C
r1  1  2  3  NaN
r1  5  6  7  8.0
```

### query
For column based query, you can simply use something like `df[colName]`:

```python
>>> df['A']                    # you'll get a series
r1    1
r1    5
Name: A, dtype: int64

>>> df[['B', 'A']]             # query multiple columns, the result is a dataframe
    B  B  A
r1  2  3  1
r1  6  7  5
```

For index based query, you can use something like `df.loc[idx]`.

- df.loc[idx]: get a series
- df.loc[[idx]]: get a dataframe that has only one series
- df.loc[[idx1, idx2]]: get dataframe multiple rows

Select a subset of a dataframe:

```python
>>> df1.loc[['X', 'Z'], ['A', 'B']]          # first array is index, second is columns
   A  B
X  1  2
Z  7  8
>>> df1.loc[:, ['B', 'A']]                   # : means all index/columns
   B  A
X  2  1
Y  5  4
Z  8  7
```

There is also a `df.iloc` selector which use index number to select. For example:

```python
>>> df1.iloc[:2, 1:]                         # index num < 2 and column num >= 1
   B  C
X  2  3
Y  5  6
>>>
```

You can use `df.loc['Z'].A` or `df.A.Z` to select value a single cell. Attention, this
 is different from `df.loc[['Z'], ['A']]` which get a `1X1` dataframe.

`pandas` supports flexible expression to filter data, like:

```python
>>> df1[ (df1.A >1) & (df1.B < 80) ]
   A  B  C     D
Y  4  5  6 -10.0
Z  7  8  9   NaN
```

Attention, you need to use `&` or `|` but not `&&`, `||`, `and`, `or` to do logical
 operations. And you need `()` since `&` has higher precedence than `>` and `<`.


### concat
You can concat two dataframes vertically or horizontally. The two dataframes needn't
 to have same index or columns. Index and columns will be unioned. Missing cells will
 be filled with `NaN`.

```python
>>> df1 = pd.DataFrame([[1,2,3], [4,5,6], [7,8,9]], index=list('XYZ'), columns=list('ABC'))
>>> df2 = pd.DataFrame([[11,12,13], [14,15,16], [17,18,19]], index=list('XYW'), columns=list('BCD'))

>>> pd.concat([df1, df2])         # vertically concat by default (axis=0)
     A   B   C     D
X  1.0   2   3   NaN
Y  4.0   5   6   NaN
Z  7.0   8   9   NaN
X  NaN  11  12  13.0
Y  NaN  14  15  16.0
W  NaN  17  18  19.0

>>> pd.concat([df1, df2], axis=1)     # horizontally using axis=1
     A    B    C     B     C     D
W  NaN  NaN  NaN  17.0  18.0  19.0
X  1.0  2.0  3.0  11.0  12.0  13.0
Y  4.0  5.0  6.0  14.0  15.0  16.0
Z  7.0  8.0  9.0   NaN   NaN   NaN
```

### calculation
You can do simple calculations like `df1.A - df2.B` or `df1.A + df2.C`. Just like `pd.concat`,
 missing cells were filled with NaN.
