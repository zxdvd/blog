```metadata
tags: postgres, sql, insert
```

## sql: use result of one insert in another insert

Sometimes, you may need to insert multiple tables at some time. And the following
 insert needs the inserted id of the previous insert.

For example, there is a school table and class table. I need to insert a new school
 and copy all classes from an existed school.

It's very easy to deal it using program language. Following is pseudo code:

```js
    const classes = await models.Class.findAll({ where: {id: schoolId } })
    const newSchool = await models.School.insert({ name: schoolName })  // insert a new school
    classes.forEach(class => {
        class.schoolId = newSchool.id       // replace schoolId with inserted school id
    })
    await models.Class.BulkCreate(classes)  // copy classes
```

But how to do it using sql? Of course, we can write producure using plpgsql or python or
 js. But can we do it with simple sql?

### insert returning id in with clause
We know that postgres has syntax like `insert into table xxxxxx returning *` so that we
 can get inserted rows. But how to use it?

Actually, postgres supports using the `insert into table xxxxxx returning *` in the `with`
 clause. So we can achieve same result using following sql

```sql
with new_school as (
    insert into school (name, created_at) values ('a_new_school', now()) returning id
),
new_class as (
    select t1.class_name, t2.id as new_school_id
    from class t1
    inner join new_school t2 on true
    where t1.school_id = 100
)
insert into class (class_name, school_id) select class_name,new_school_id from new_class;
```

The `update table_name xxxx returning *` is also supported. You can use multiple insert
 or update clauses if you want.

### use nextval(sequence) to get id
You can use `nextval(sequence_name)` to get a determined id and reuse it later. Just like
 following:

```sql
with new_school(id,name,created_at) as (
    values (nextval('school_id_seq'), 'new1', now()), (nextval('school_id_seq'), 'new1', now())
),
new_class as (
    select t1.class_name, t2.id as new_school_id
    from class t1
    inner join new_school t2 on true
    where t1.school_id = 100
),
insert1 as (
    insert into school (id,name,created_at) select * from new_school
)
insert into class (class_name, school_id) select class_name,new_school_id from new_class;
```

You may wonder whether the `nextval` will be evaluated each time you use the `new_school`.
Actually, it won't, it is only evaluate once and the `new_school` is like a materialized
 table. And you can use a simple sql to verify this:

```sql
with new_school(id,name,created_at) as (
    values (nextval('school_id_seq'), 'new1', now())
),
insert1 as (
    insert into school (id,name,created_at) select * from new_school
),
insert2 as (
    insert into school (id,name,created_at) select * from new_school
)
insert into school (id,name,created_at) select * from new_school;
```

Above sql will insert 3 times. If you get 3 different id, then it indicates that `nextval`
 is evaluated 3 times. If you get 3 same ids, then it verifies that it is only evaluated
 once and the result is materialized. And actually, you'll get same ids.
