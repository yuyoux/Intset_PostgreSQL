---------------------------------------------------------------------------
--
-- complex.sql-
--    This file shows how to create a new user-defined type and how to
--    use this new type.
--
--
-- Portions Copyright (c) 1996-2017, PostgreSQL Global Development Group
-- Portions Copyright (c) 1994, Regents of the University of California
--
-- src/tutorial/complex.source
--
---------------------------------------------------------------------------

-----------------------------
-- Creating a new type:
--	We are going to create a new type called 'complex' which represents
--	complex numbers.
--	A user-defined type must have an input and an output function, and
--	optionally can have binary input and output functions.  All of these
--	are usually user-defined C functions.
-----------------------------

-- Assume the user defined functions are in /srvr/z5143390/postgresql-10.4/src/tutorial/complex$DLSUFFIX
-- (we do not want to assume this is in the dynamic loader search path).
-- Look at $PWD/complex.c for the source.  Note that we declare all of
-- them as STRICT, so we do not need to cope with NULL inputs in the
-- C code.  We also mark them IMMUTABLE, since they always return the
-- same outputs given the same inputs.

-- the input function 'complex_in' takes a null-terminated string (the
-- textual representation of the type) and turns it into the internal
-- (in memory) representation. You will get a message telling you 'complex'
-- does not exist yet but that's okay.

CREATE FUNCTION intset_in(cstring)
   RETURNS intSet
   AS '/srvr/z5143390/postgresql-10.4/src/tutorial/intset'
   LANGUAGE C IMMUTABLE STRICT;

-- the output function 'complex_out' takes the internal representation and
-- converts it into the textual representation.

CREATE FUNCTION intset_out(intSet)
   RETURNS cstring
   AS '/srvr/z5143390/postgresql-10.4/src/tutorial/intset'
   LANGUAGE C IMMUTABLE STRICT;


-- now, we can create the type. The internallength specifies the size of the
-- memory block required to hold the type (we need two 8-byte doubles).

CREATE TYPE intSet (
   internallength = 20,
   input = intset_in,
   output = intset_out
   
);


-----------------------------
-- Using the new type:
--	user-defined types can be used like ordinary built-in types.
-----------------------------

create table Features (
    id int primary key,
    name text
);

create table DBSystems (
    name text,
    features intSet
);

-- data for user-defined types are just strings in the proper textual
-- representation.

insert into Features (id, name) values
    (1, 'well designed'),
    (2, 'efficient'),
    (3, 'flexible'),
    (4, 'robust');
insert into DBSystems (name, features) values
    ('MySQL', '{}'),
    ('MongoDB', '{}'),
    ('Oracle', '{2,4}'),
    ('PostgreSQL', '{1,2,3,4}');

SELECT * FROM DBSystems;

create table mySets (id integer primary key, iset intSet);

insert into mySets values (1, '{1,2,3}');
insert into mySets values (2, '{1,3,1,3,1}');
insert into mySets values (3, '{3,4,5}');
insert into mySets values (4, '{4,5}');
select * from mySets;

-----------------------------
-- Creating an operator for the new type:
--	Let's define an add operator for complex types. Since POSTGRES
--	supports function overloading, we'll use + as the add operator.
--	(Operator names can be reused with different numbers and types of
--	arguments.)
-----------------------------


CREATE FUNCTION intset_contains(int, intSet)
   RETURNS bool
   AS '/srvr/z5143390/postgresql-10.4/src/tutorial/intset'
   LANGUAGE C IMMUTABLE STRICT;

-- we can now define the operator. We show a binary operator here but you
-- can also define unary operators by omitting either of leftarg or rightarg.
CREATE OPERATOR <@ (
   leftarg = int,
   rightarg = intSet,
   procedure = intset_contains
   -- commutator = <@
);

--select a.* from mySets a
--where (1 <@ a.iset);

-- first, define the required operators
CREATE FUNCTION intset_cardinality(intSet) RETURNS int
   AS '/srvr/z5143390/postgresql-10.4/src/tutorial/intset' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION intset_containset(intSet, intSet) RETURNS bool
   AS '/srvr/z5143390/postgresql-10.4/src/tutorial/intset' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION intset_equal(intSet, intSet) RETURNS bool
   AS '/srvr/z5143390/postgresql-10.4/src/tutorial/intset' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION intset_intersection(intSet, intSet) RETURNS intSet
   AS '/srvr/z5143390/postgresql-10.4/src/tutorial/intset' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION intset_union(intSet, intSet) RETURNS intSet
   AS '/srvr/z5143390/postgresql-10.4/src/tutorial/intset' LANGUAGE C IMMUTABLE STRICT;




CREATE OPERATOR @ (
   rightarg = intSet, procedure = intset_cardinality
   -- commutator = @
   -- restrict = scalarltsel, join = scalarltjoinsel
);
CREATE OPERATOR @> (
   leftarg = intSet, rightarg = intSet, procedure = intset_containset,
   commutator = <@
   -- restrict = scalarltsel, join = scalarltjoinsel
);
CREATE OPERATOR = (
   leftarg = intSet, rightarg = intSet, procedure = intset_equal,
   commutator = = ,
   -- leave out negator since we didn't create <> operator
   -- negator = <> ,
   restrict = eqsel, join = eqjoinsel
);
CREATE OPERATOR && (
   leftarg = intSet, rightarg = intSet, procedure = intset_intersection,
   commutator = &&& 
   -- restrict = scalargtsel, join = scalargtjoinsel
);
CREATE OPERATOR || (
   leftarg = intSet, rightarg = intSet, procedure = intset_union
   -- commutator = &&& 
   -- restrict = scalargtsel, join = scalargtjoinsel
);



-- now we can make the operator class
-- CREATE OPERATOR CLASS intset_ops
--    DEFAULT FOR TYPE intset USING btree AS
--        OPERATOR        1       <@,
--        OPERATOR        2       @,
--        OPERATOR        3       @>,
--        OPERATOR        4       =,
--        OPERATOR        5       &&,
--	  OPERATOR	  6	  ||;

--select a.*, b.* from mySets a, mySets b
--where (b.iset @> a.iset) and a.id != b.id;

--update mySets set iset = iset || '{5,6,7,8}' where id = 4;

--select a.*, b.* from mySets a, mySets b
--where (a.iset @> b.iset) and a.id < b.id;

--select id, iset, (@iset) as card from mySets order by id;

--select a.iset, b.iset, a.iset && b.iset
--from mySets a, mySets b where a.id < b.id;

--delete from mySets where iset @> '{1,2,3,4,5,6}';

--select * from mySets;






