<!---
tags: postgres, string, syntax
-->

In postgres, single quoted string is totally different from double quoted string. Double quoted string 
is used for database object like table, column, index, procedure and so on while single quoted string 
is used for user input, function paramters, operator paramters. Example:

```sql
select "t"."a", t."b" from "public".table1 where "a" like '%hello%';
select 'hello' || ' ' || 'world';
```

#### how to deal with special characters like ',",\?
For " and \ just treat them as normal characters. But for **'**, we need to escape it with two **'** thus
**''** (it's TWO single quotes NOT ONE double quotes).

To defend sql injection, we can replace all **'** with **''** for user input string like name or keyword.

#### how about characters like \n, \r, \01 ?
From above, we know that the `\n` in `'hello \n'` is just two characters (`\` and `n`) not the one that means
newline. So how about \n (newline), \r (tab) or even \01, \02?
Just add a `E` or `e` before the single quotes. Example:

```sql
select E'hello \n word' as a, e'hi \t \01 \t tab' as b;
```

#### how about unicode code points?
Add **u&** before string, e.g. `u&'\FFFF\EEEE'`, FFFF or EEEE is 4 bytes hex of unicode code point.

```sql
select u&'\6211们';      -- 我们
select u&'#6211们' uescape '#';      -- you change \ to other character using uescape
```

#### dollar quoted string
It's really boring if there is too much single quotes to escape. Then postgres supports the dollar quotes 
to deal with this. We often see it in procedures but may not aware that they are almost same as single quotes

``` sql
create function test1() returns int as $$
        select 10;
$$ language sql;

do $mydo$
begin
    raise notice 'hello';
    raise notice $mystr$hello$mystr$;      -- you can have another dollar quotes
end $mydo$ language plpgsql;
```

Actually, you can replace `$$` or `$mydo$` to `'`, but then you need to escape all other `'` between them.
So `$TAG$` is just another form of `'` to help you avoid of too much single quotes escaping.

You can use `$$`, or add other tag between it like `$function$`, `tag123` or others. Some attentions:

    * the body between it should not contain SAME `$$`
    * begin and end tags should match exactly (case senstive)




