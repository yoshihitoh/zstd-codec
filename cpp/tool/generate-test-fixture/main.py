import datetime
import json
import mimesis
import random
import typing
import tqdm


class Author(object):
    """著者"""

    def __init__(self, author_id: int):
        self.id = author_id         # type: int
        self.first_name = None      # type: str
        self.last_name = None       # type: str


class Book(object):
    """書籍"""

    def __init__(self, book_id: int):
        self.id = book_id           # type: int
        self.title = None           # type: str
        self.author = None          # type: Author
        self.isbn = None            # type: str
        self.release_at = None      # type: datetime.date


def next_author(data_locale: str) -> typing.Iterator[Author]:
    personal = mimesis.Person(data_locale)
    author_id = 0

    while True:
        author_id += 1

        author = Author(author_id)
        author.first_name = personal.name()
        author.last_name = personal.surname()

        yield author


def next_book(authors: typing.List[Author], data_locale: str) -> typing.Iterator[Book]:
    code = mimesis.Code(data_locale)
    date = mimesis.Datetime(data_locale)
    text = mimesis.Text(data_locale)

    book_id = 0

    while True:
        book_id += 1

        book = Book(book_id)
        book.title = text.title()
        book.author = random.choice(authors)
        book.isbn = code.isbn()
        book.release_at = date.date(start=1800, end=2018)

        yield book


class SimpleJsonEncoder(json.JSONEncoder):
    """JSONエンコーダ、オブジェクトの __dict__ の内容でエンコードする"""

    def default(self, o):
        if isinstance(o, datetime.date):
            return o.strftime('&m/%d/%Y')
        else:
            return o.__dict__


def generate_books(data_locale: str, n_authors: int, n_books: int) -> typing.Iterator[Book]:
    # 著者 n_authors人
    author_generator = next_author(data_locale)
    authors = [next(author_generator) for _ in range(n_authors)]

    # 書籍 n_books冊
    book_generator = next_book(authors, data_locale)
    return (next(book_generator) for _ in range(n_books))


def main():
    data_locale = 'ja'
    n_authors = 10
    n_books = 10_000
    books = generate_books(data_locale, n_authors=n_authors, n_books=n_books)
    encoder = SimpleJsonEncoder()

    with open(f'books_{data_locale}.json', 'w') as wf:
        for b in tqdm.tqdm(books):
            print(encoder.encode(b), file=wf)


if __name__ == '__main__':
    main()
