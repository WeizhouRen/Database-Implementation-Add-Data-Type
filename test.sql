delete from mySets;
insert into mySets values (1, '{1,2,3}');
insert into mySets values (2, '{1,3,1,3,1}');
insert into mySets values (3, '{3,4,5}');
insert into mySets values (4, '{4,5}');
select * from mySets order by id;
select a.*, b.* from mySets a, mySets b where (b.iset @< a.iset) and a.id != b.id;
update mySets set iset = iset || '{5,6,7,8}' where id = 4;
select * from mySets where id=4;
select a.*, b.* from mySets a, mySets b where (b.iset @< a.iset) and a.id != b.id;
select id, iset, (#iset) as card from mySets order by id;
select a.iset, b.iset, a.iset && b.iset from mySets a, mySets b where a.id < b.id;
delete from mySets where iset @< '{1,2,3,4,5,6}';

select # '{1, 3, 9, 26, 5, 13, 31, 15, 27, 81, 22, 45, 92, 20, 50, 145, 46, 89, 32, 71, 151, 40, 75, 163, 73, 124, 60, 126, 244, 97, 219, 63, 132, 306, 68, 144, 297, 79, 166, 354, 83, 187, 394, 94, 203, 419, 108, 220, 460, 127, 260, 110, 247, 513, 161, 340, 117, 252}'::IntSet;

select * from mySets;
delete from mySets;
insert into mySets values (1, '{a,b,c}');
insert into mySets values (2, '{ a, b, c }');
insert into mySets values (3, '{1, 2.0, 3}');
insert into mySets values (4, '{1, {2,3}, 4}');
insert into mySets values (5, '{1, 2, 3, 4, five}');
insert into mySets values (6, '{ 1 2 3 4 }');
insert into mySets values (7, ' 1 2 3 4 ');
insert into mySets values (8, ' 1 2 3 4 }');
insert into mySets values (9, '{ -1 }');
insert into mySets values (10, '{1,2,3 ');
insert into mySets values (11, '1,2,3,4,5');
insert into mySets values (12, '{{1,2,3,5}');
insert into mySets values (13, '{7,17,,27,37}');
insert into mySets values (14, '{1,2,3,5,8,}');



