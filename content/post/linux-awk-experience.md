+++
title = "awk experience"
date = "2015-05-16"
categories = ["Linux"]
tags = ["linux", "awk"]
slug = "awk-experience"
Authors = "Xudong"
draft = true
+++

I didn't like the awk and sed when I began to learn linux. I think they are very
hard to learn and I don't need. However I finally found that it really helpful
in some situations and now I want to share my experience about it.

Let's assume there is a file test.txt:

    Name       Sex    Age    Weight   Grade   Score
    Zhangsan   Male   10     35       3       81
    Lisi       Male   9               3       85
    Wangwu     Female 11     38       4       83
    Hanmei     Female 10     36       4       88
    Lucy       Female 12              5       82

### Get name, Sex and Age

    :::shell
    #$1 is the first column, $2 is the second, $0 is the total line
    >awk  '{print $1, $2, $3}'  test.txt

### Get name, score and sex (switch order of score and sex)

    :::shell
    >awk '{print $1, $NF, $2}'  test.txt
    #$NF is the last column, in this case you'd better use $NF since there is no
    #weight data of Lisi and Lucy
    #If you want to show column Grade you could use **$(NF-1)**. You must not use $5
    #otherwise you'll find that 85 (which is score of Lisi) will become grade of
    #Lisi.

### Get all columns but switch column Sex and Age

    :::shell
    >awk '{t=$2; $2=$3; $3=t; print}' test.txt
    #The single print equals **print $0**

## Options and Variables

### Input Field Separator

Sometimes, the input file may be separated by other characters like **=-;:,**. You
need to specify the separated characters (The separator could be more than one
characters, like **abc**, **on** ). Example:

    :::shell
    >awk  -F':'  '{print $1, $NF}'  /etc/passwd
    >mount | column  -t | awk  -F' on '  '{print $1}'
    #In the above example, the separator is ' on '.

### Output Field Separator

Just like the input field separator, you may want to separate the output column
with another charachter or words. Using `OFS='SEPARATOR'` like following:

    :::shell
    #Get name and sex, separate with **;**
    >awk  '{print $1,$2}'  OFS=';'  test.txt

### NR-Number of Records (line number)

NR is the line number. You can use it to limit range of line of ignore top
lines.

    :::shell
    >awk  'NR>1 && NR<=4'  test.txt          //get line 2,3,4
    >awk  '{ if(NR>1 && NR<=4) print }'  test.txt       //equals above one

## Patterns

1. You can use patterns to filter line. There are different kinds of patterns.
   Like `/Male/` is a regular expression one. `$2=="Male"` is a normal expression.
   And also the range pattern like `'/begin/,/end/'`.

        :::shell
        >awk  '/Female/'  test.txt     //just like grep
        >awk  '/Female/ {print $1, $2}'  test.txt
        >awk  '$2=="Female"  {print $1, $2}'  test.txt    //need to quote Female
        #Get girls who is in grade 4
        #You can use ||, && to combine multiple patterns
        >awk  '$2=="Female"  &&  $(NF-1)==4 || $NF > 80'  test.txt

2. Range pattern is a special pattern to match a block of text. It contains two
   patterns in the form of `'begin-pattern, end-pattern'`. The two patterns
   could be either regular one or normal one. Examples:

        :::shell
        #Get lines from Lisi to Hanmei
        >awk  '/Lisi/, /Hanmei/'  test.txt
        >awk  '/Lisi/, $1== "Hanmei" '  test.txt   //equals to above one
        #lspci  -v | awk  '/Network, /^$/'         //block from Network to blank line

## How does it work?

After some simple examples, I'd like to explain a little about how does it work.
Following is a sample of expressions that awk received and processed.

    :::text
    BEGIN               { ACTION }
            pattern     { ACTION }
            pattern     { ACTION }
            ...
            pattern     { ACTION }
    END                 { ACTION }

The begin action will only execute once before awk starts to process lines while
end action will execute once after it finishes all lines.

The "pattern {ACTION}" is used most frequently. You could have as many
pattern-action as you want. Take `awk '/Female/ {print $1} test.txt'` as
example, `/Female/` is pattern while `{print $1}` is action.

Awk reads a line from the file. If it matches the pattern, it will execute the
action following. And then it will continue to match with second pattern, it
will execute second action if matched. And then third pattern and action. If all
pattern-action is processed. It will read the next line and process it.

So if you try with `awk '{print} {print}' test.txt`, it will print every line
twice.

Sometimes you may see some patterns with action, just like `awk '/Female/'
test.txt`. In these cases, a default action `{print}` is executed--that means
the matched line is printed.


## Advanced examples:

### Calculations

You can use awk to do some calculations.

    :::shell
    #Get sum of all scores
    >awk  '{sum+=$NF} END {print sum}'  test.txt

### Another way to get block of text

I've showed about get block of text via range pattern, like `lspci -v | awk
'/Network/, /^$/'`. Now I'll show another way using variable.

    :::shell
    >lspci -v | awk '/Network/ {f=1}  /^$/ {f=0}  f'
    #the **f** is the last pattern, it doesn't specify a action, so use the
    #default one--thus **{print}**. So it equals to the following:
    >lspci -v | awk '/Network/ {f=1}  /^$/ {f=0}  f {print}'

    Explantation:
    pattern     action
    /Network/   {f=1}       ;set variable f to 1 if Network matched in current line
    /^$/        {f=0}       ;set f to 0 if current line if blank
    f           {print}     ;if f is true, print current line

You need to take case that the variable f is global. If the N line matched
/Netowork/, f will be set to 1 and kept. That means f will always be 1 since
then. So f will be 1 in N+1, N+2, N+3... line if they are not blank.
Similar to it, f will be set to 0 and kept if blank line matched.

### Keyword "next"

The **next** keyword will stop parsing in the current line and will read the
next line and parse it. (like continue in loop of python). Example:

    :::shell
    >lspci -v | awk '/Network/ {f=1;next}  /^$/ {f=0}  f'
    #if Network matched, it will set f to 1 and stop to run the left (/^$/ {f=1} f)
    #so the last **f** doesn't take effect which means current line won't print
    #without next the line matched Network will be printed

## Replace block with another file

I met a situation that I want to to replace a block of text with content of
another file (the file stores some secret keys that I don't want to commit to
github).

    :::shell
    >cat sample.txt | awk '/^begin/ { f=1; while (getline < "key.txt") {print} } /end/ {f=0} !f'
    #getline return 1 if get a line, 0 if get end of line and -1 if cann't open file
    # !f is a boolean pattern

## Reference:

1. [GNU manual](https://www.gnu.org/software/gawk/manual/html_node/index.html#SEC_Contents)
2. [awk tutorial](http://www.grymoire.com/Unix/Awk.html)
