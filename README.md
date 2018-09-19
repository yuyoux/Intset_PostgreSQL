# Intset_PostgreSQL

To implement a new data type called Intset for PostgreSQL, complete with input/output functions and a range of operations. 

#IntSet values
In mathematics, we represent a set as a curly-bracketed, comma-separated collection of values: {1,2,3,4,5}.Such a set contains only distinct values. No particular ordering can be imposed.

Our intSet values can be represented similarly: we can have a comma-separated list of integers, surrounded by a set of curly braces, which is presented to and by PostgreSQL as a string: For example, '{ 1, 2, 3, 4, 5 }'.

Whitespace should not matter, so '{1,2,3}' and '{ 1, 2, 3 }' are equivalent. Similarly, a set contains distinct values, so '{1,1,1,1,1}' is equivalent to '{1}'. And ordering is irrelevant, so '{1,2,3}' is equivalent to '{3,2,1}'. 

#Operations
i <@ S
    intSet S contains the integer i; that is, i∈S
    .
@ S
    give the cardinality, or number of distinct elements in, intSet S; that is, |S|
    .
A @> B
    does intSet A contain only values in intSet B? that is, for every element of A, is it an element of B? (A⊂B
    )
A = B
    intSets A and B are equal; that is, intSet A contains all the values of intSet B and intSet B contains all the values of intSet A, or, every element in A can be found in B
    , and vice versa.
A && B
    takes the set intersection, and produces an intSet containing the elements common to A and B; that is, A∩B
    .
A || B
    takes the set union, and produces an intSet containing all the elements of A and B; that is, A∪B
    .
A !! B
    takes the set disjunction, and produces an intSet containing elements that are in A and not in B, or that are in B and not in A
    .
A - B
    takes the set difference, and produces an intSet containing elements that are in A and not in B. Note that this is not the same as A !! B.
