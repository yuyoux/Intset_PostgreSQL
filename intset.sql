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

--INSERT INTO test_complex VALUES ('(1.0, 2.5)', '(4.2, 3.55 )');
--INSERT INTO test_complex VALUES ('(33.0, 51.4)', '(100.42, 93.55)');

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




