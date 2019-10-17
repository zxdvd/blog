
import os

from urllib.parse import quote

def walk_dir(folder):
    posts = []
    with os.scandir(folder) as it:
        for entry in it:
            if entry.is_dir():
                yield from walk_dir(entry.path)
            if entry.is_file():
                yield entry

def gen_index(folder, template, target, placeholder):
    links = []
    for entry in walk_dir(folder):
        if not entry.name.endswith('.md'):
            continue
        name = entry.name[:-3]
        link = './' + entry.path
        post = '* [%s](%s)' % (name, quote(link))
        links.append(post)
    index = '\n'.join(links)
    with open(template, 'r') as f1, open(target, 'w') as f2:
        content = f1.read()
        content = content.replace(placeholder, index)
        f2.write(content)

if __name__ == '__main__':
    folder = os.getenv('folder', '.')
    template = os.getenv('template', 'README.md.tmpl')
    target = os.getenv('target', 'README.md')
    placeholder = os.getenv('placeholder', '{{__index__}}')
    gen_index(folder, template, target, placeholder)
