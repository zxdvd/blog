+++
Title = "Summary: the curl bugzilla project"
Date = "2014-12-12"
Categories = "Python"
Tags = ["python"]
Slug = "summary-curl-bugzilla"
Authors = "Xudong"
+++

I started a small project based on tornado and mongodb last week, now I'd like
to write a summary about it since most functions are finished.

### Why start this project

1. We used bugzilla frequently but the traffic to bugzilla is very slow.

2. It uses openid to login and the authentication cookies last very short.

3. Too many irrelevant products.

### Goal of the project

1. Using curl to search the bugzilla easily. You can search by keywords, or email.

2. Can easily get latest reported bugs of a specific product.

### How to?

1. Our bugzilla enabled the xmlrpc api so it's very easy to fetch data using
 xmlrpcclient library. I chose to store data with mongodb.

2. Wrote the web server based on tornado. Parsed the requests and queried the
   mongodb and then wrote back the data.

Code repo:
[github.com/zxdvd/scripts/tree/master/bugzilla](https://github.com/zxdvd/scripts/tree/master/bugzilla)

I didn't use a independent repo since there isn't too much codes.

<!--more-->

### Result

    > curl 147.2.212.204/gnome
    866644 @lupe.amezquita sles12   | [HP HPS Bug] SLES12 is not showing the mounted
    (via iLO virtual media) virtual folder or images as media in desktop GUI page
    881245 @whdu           sled12   | Update package "update-test-affects-package-manager"
    from "Software Update" trigger relogin instead of restarting gpk-update-viewer
    864872 @fcrozat        sled12   | gnome-shell lock-screen should react on simple
    click
    846028 @oneukum        sled11sp3| gnome-power-manager without warning adds a week
    to wakeup date
    818242 @hpj            sled11sp3| Evince hangs on specific PDF

### What I learned?

#### Libraries

1. **xmlrpc.client**

        from xmlrpc.client import ServerProxy

        proxy = ServerProxy(uri, use_datetime=True)
        bugs = proxy.Bug.search({'product': ['product you want to fetch']})

    - The uri could be like this: `https://username:passwd@api.xxx.com`
    - Using `use_datetime=True` option, it will convert the time to class
      `datetime.datetime`. Otherwise, you'll get a `xmlrpc.client.DateTime`
      instance and it'll be not recognized by json module and pymongo module.

2. **click**

    I'd like to use this third party library instead of the standard argparse
because it's simple and easy to use and it has nice and rich documentation. I
used it to parse the username and password of the bugzilla (It's not necessary
if you embedded username and password in the url).

3. **pymongo**

    The python interface of the mongodb.

        :::python
        client = MongoClient('mongodb://IP_ADDRESS:27017/')
        db = client.bz                   //bz is name of the db
        prod_col = db.prods              //prods is name of collection
        prod_col.save(item)              //dict type contains data of a bug

    bz and prods are names I gave to db and collections, it could be any other names
if you like.

4. **tornado**

    I've ever learned and used django and flask. Now I want to try another cool web
framework--tornado, it's famous for its asynchronization. The document is not so
rich as django but I think it's good and enough. You can get started with it
quickly.

#### Miscellaneous

1. **terminal color**

    I want each column could be distinguished by different color. I learnt a
    little about ternimal formatting. So I need to add some prefix strings to change
    font color or background color or other effects. Examples:

    `echo -e "\033[31mHello World\033[0m"` will change font color to red. The
    `\033[31m` will change all behind it to red and `\033[0m` will reset all to
    default. (You could replace \033 with \e in shell)

    In python, it should be like `print('\033[31mHello World\033[0m')`. But
mixing them together looks a little ugly. So I wrapped needed color prefix
strings to a dict and then used the format method of string. Examples:

        :::python
        term_color = {'blk': '\033[30m', 'green': '\033[32m',
                      'red': '\033[31m', 'reset': '\033[0m'}
        p = {'name': 'John', 'age': '20'}
        p.update(term_color)
        print('I am {red}{name}{reset}, I am {age} years old'.format(**p))

    [**Ref: bash terminal color and
    formatting**](http://misc.flogisoft.com/bash/tip_colors_and_formatting)
