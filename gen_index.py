
import os
import subprocess

from pathlib import Path

from urllib.parse import quote

def get_all_git_files(path='.'):
    out = subprocess.run(['git', 'ls-files', '-z', path], capture_output=True, encoding='utf8')
    return out.stdout.split('\0')

def get_articles(folder='content', ext='.md'):
    articles = get_all_git_files(folder)
    articles = [a for a in articles if a.endswith(ext)]
    articles.sort()
    return articles

def gen_link(filename):
    link = quote('./' + filename)
    category = os.path.basename(os.path.dirname(filename))
    basename = os.path.splitext(os.path.basename(filename))[0]
    name = basename
    if category:
        name = '%s / %s' % (category, basename)
    return '* [%s](%s)' % (name, link)

def gen_index(folder, template_content, placeholder, target='README.md', ext='.md'):
    articles = get_articles(folder, ext)
    if not articles:
        return
    links = map(gen_link, articles)

    index = '\n'.join(links)
    with open(target, 'w') as f2:
        content = template_content.replace(placeholder, index)
        f2.write(content)

if __name__ == '__main__':
    folder = os.getenv('folder', 'content')
    template = os.getenv('template', 'README.md.tmpl')
    with open(template, 'r') as f:
        template_content = f.read()
    placeholder = os.getenv('placeholder', '{{__index__}}')

    gen_index(folder, template_content, placeholder)

    cwd = os.getcwd()
    for p in Path('content').iterdir():
        if not p.is_dir():
            continue
        os.chdir(p)
        gen_index('.', template_content, placeholder)
        os.chdir(cwd)
