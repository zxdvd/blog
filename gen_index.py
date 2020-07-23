
import os
import subprocess

from urllib.parse import quote

def get_all_git_files():
    out = subprocess.run(['git', 'ls-files', '-z'], capture_output=True, encoding='utf8')
    return out.stdout.split('\0')

def get_articles(folder='content', ext='.md'):
    articles = get_all_git_files()
    articles = [a for a in articles if a.startswith(folder) and a.endswith(ext)]
    articles.sort()
    return articles

def gen_link(filename):
    link = quote('./' + filename)
    category = os.path.basename(os.path.dirname(filename))
    basename = os.path.splitext(os.path.basename(filename))[0]
    name = '%s / %s' % (category, basename)
    return '* [%s](%s)' % (name, link)

def gen_index(folder, template, target, placeholder, ext='.md'):
    articles = get_articles(folder, ext)
    links = map(gen_link, articles)

    index = '\n'.join(links)
    with open(template, 'r') as f1, open(target, 'w') as f2:
        content = f1.read()
        content = content.replace(placeholder, index)
        f2.write(content)

if __name__ == '__main__':
    folder = os.getenv('folder', 'content')
    template = os.getenv('template', 'README.md.tmpl')
    target = os.getenv('target', 'README.md')
    placeholder = os.getenv('placeholder', '{{__index__}}')

    gen_index(folder, template, target, placeholder, ext='.md')
