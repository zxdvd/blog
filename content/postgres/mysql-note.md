### some mysql notes

#### string comparison
Mysql will ignore the trailing spaces while comparing strings using =.
All of following fields are TRUE

    select 'a' = 'a', 'a' = 'a ', 'a' = 'a  ';
To avoid this, you may try to query like:

    select * from user where name = 'Tom' and length(name) = length('Tom');

And it uses case insenstive comparing for default character set. Then
 if you want case senstive comparing you need to specify `COLLATE` or
 using binary comparing like:

    select * from user where binary name = 'Tom';
    select 'aaa' COLLATE utf8_bin = 'AAA', 'aaa' = 'AAA';
