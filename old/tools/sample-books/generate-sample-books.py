# -*- coding: utf-8 -*-


import json
import os
import random
from datetime import datetime

from mimesis import Code, Datetime, Personal, Text


NOW = datetime.now()


class Fakers:
    CODE = Code('en')
    DATETIME = Datetime('en')
    PERSON = Personal('en')
    TEXT = Text('en')


class Author(object):
    def __init__(self, author_id):
        self.id = author_id
        self.first_name = None
        self.last_name = None

    def __repr__(self):
        return f'Author({self.first_name} {self.last_name})'


class Book(object):
    def __init__(self, book_id):
        self.id = book_id
        self.title = None
        self.author = None
        self.isbn = None
        self.release_at = None

    def __repr__(self):
        return f'Book(id={self.id}, title={self.title}, author={self.author})'


def _is_any_instanceof(o, *types):
    return any(isinstance(o, t) for t in types)


class GenericObjectJsonEncoder(json.JSONEncoder):
    def default(self, o):
        if isinstance(o, datetime):
            return o.isoformat()
        elif _is_any_instanceof(o, Author, Book):
            return o.__dict__ if o else {}
        else:
            raise ValueError(f'unsupported data type: {type(o)}')


def _create_author(author_id):
    author = Author(author_id)
    author.first_name = Fakers.PERSON.surname()
    author.last_name = Fakers.PERSON.last_name()

    return author


def _create_book(sequence_no, author):
    book = Book(sequence_no)
    book.title = Fakers.TEXT.title()
    book.author = author
    book.isbn = Fakers.CODE.isbn()
    book.release_at = Fakers.DATETIME.date(start=1970, end=NOW.year)

    return book


def next_author():
    authors = dict([(x.id, x) for x in map(_create_author, range(10000))])

    while True:
        indexes = list(range(len(authors)))
        random.shuffle(indexes)

        for i in indexes:
            yield authors.get(i)


def next_book():
    next_id = 1
    gen_authors = next_author()

    while True:
        next_id += 1
        yield _create_book(next_id, next(gen_authors))


def enumerate_books(total_size, chunk_size):
    total_count = 1_000_000
    gen_book = next_book()
    done = 0
    while done < total_count:
        use_fully = done + chunk_size <= total_size
        take_size = chunk_size if use_fully else total_size - chunk_size
        done += take_size

        books = [next(gen_book) for _ in range(take_size)]
        yield books


def create_sample_data(sequence_no):
    encoder = GenericObjectJsonEncoder()

    dest_dir = os.path.join('.', 'sample')
    if not os.path.exists(dest_dir):
        os.makedirs(dest_dir)

    with open(os.path.join('.', 'sample', f'sample-books-{sequence_no}.json'), 'w') as wf:
        progress = 0
        for books in enumerate_books(total_size=10_000_000, chunk_size=10_000):
            print('\n'.join(encoder.encode(x) for x in books), file=wf)
            print('', file=wf)

            progress += len(books)
            if progress % (10_000 * 1) == 0:
                print(f'{progress:8} rows processed...')


def main():
    for x in range(10):
        create_sample_data(x)


if __name__ == '__main__':
    main()
