```metadata
tags: python, orm, sqlalchemy
```

## sqlalchemy tips


### relationship
#### define relationship without foreign key
It's easy to set relationship of two tables using foreign key. I'd like to set relationship
 without foreign key. It has better performance and is easy to partition. You can setup
 relationship at ORM level only (no foreign key) like following:

```python
from sqlalchemy.orm import relationship, foreign

class Parent(Base):
    id = Column(Integer, primary_key=True)
    children = relationship(Child,
        primaryjoin = id = foreign(Child.parent_id))
```

You don't have to define it inside ORM model. You can also define ti separately outside
 of the model. Like:

```python
Parent.children = relationship(Child,
    primaryjoin = Parent.id = foreign(Child.parent_id))
```

For one to one relationship, add `uselist=False` to relationship so that it knows it is
 a one to one relationship.

    child = relationship(Child, uselist = False)

#### self join relationship
It's common to have nested data and you may need to join with itself. Like following

```python
from sqlalchemy.orm import aliased, relationship, foreign

class Person(Base):
    id = Column(Integer, primary_key=True)
    child_ids = Column(Integer)

Child = aliased(Person)

Person.children = relationship(Child,
    primaryjoin = foreign(Child.id) == func.any(Person.child_ids),
    viewonly = True,
    )
```

By this way, I use `aliased` to create an alias and it acts like another table. Then
 we turn self join to a normal join between two tables.


### postgres specific features

#### array type
Though sqlalchemy has `ARRAY` type by default. But you need to use `postgresql.ARRAY`
 to use specific features of postgres array.

```python
    from sqlalchemy.dialects import postgresql

    student_ids = Column(postgresql.ARRAY(Integer))        # define an array column
```

#### partial index
Postgres supports partial index, you can define a partial index like following:

    Index('idx_student_uniq', name, unique=True, postgresql_where=(deleted_at == None)),

#### upsert (insert on conflict)

```python
from sqlalchemy.dialects.postgresql import insert as pg_insert

insert_stmt = pg_insert(student).values(name='xxx', age=10)
do_update_stmt = insert_stmt.on_conflict_do_update(
        constraint='idx_student_uniq_name',
        set_=dict(age=age),
    )
session.execute(do_update_stmt)
```

You can also relpace constraint with unique columns. For partial unique index, you can
 use `index_where` to specify the condition.

```python
do_update_stmt = insert_stmt.on_conflict_do_update(
        index_elements = ['class_id', 'student'],
        index_where = Student.deleted_at == None,
        set_=dict(age=age),
    )
```

### references
-. [sqlalchemy doc: postgres](https://docs.sqlalchemy.org/en/14/dialects/postgresql.html)
-. [sqlalchemy doc: relationship](https://docs.sqlalchemy.org/en/14/orm/basic_relationships.html)
