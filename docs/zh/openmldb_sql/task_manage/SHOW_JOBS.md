# SHOW JOBS

`SHOW JOBS`语句用于显示在集群版下已经提交的任务列表。

```SQL
SHOW JOBS;
```

## Example

查看当前所有的任务:

```sql
SHOW JOBS;

 ---- ---------- ------- ------------ ---------- ----------- --------- ---------------- -------
  id   job_type   state   start_time   end_time   parameter   cluster   application_id   error
 ---- ---------- ------- ------------ ---------- ----------- --------- ---------------- -------
```

提交在线数据导入任务:

```sql
LOAD DATA INFILE 'file:///tmp/test.csv' INTO TABLE demo_db.t1 options(format='csv', header=false, mode='append');

---- ------------------ ----------- ------------ --------------- ---------------------------------------------------------------------------------------------------------------------------- --------- ---------------- -------
  id   job_type           state       start_time   end_time        parameter                                                                                                                    cluster   application_id   error
 ---- ------------------ ----------- ------------ --------------- ---------------------------------------------------------------------------------------------------------------------------- --------- ---------------- -------
  1    ImportOnlineData   Submitted   0            1641981373227   LOAD DATA INFILE 'file:///tmp/test.csv' INTO TABLE demo_db.t1 options(format='csv', header=false, mode='append');           local
 ---- ------------------ ----------- ------------ --------------- ---------------------------------------------------------------------------------------------------------------------------- --------- ---------------- -------
```

查看当前所有的任务:

```sql
SHOW JOBS;

---- ------------------ ----------- ------------ --------------- ---------------------------------------------------------------------------------------------------------------------------- --------- ---------------- -------
  id   job_type           state       start_time   end_time        parameter                                                                                                                    cluster   application_id   error
 ---- ------------------ ----------- ------------ --------------- ---------------------------------------------------------------------------------------------------------------------------- --------- ---------------- -------
  1    ImportOnlineData   Submitted   0            1641981373227   LOAD DATA INFILE 'file:///tmp/test.csv' INTO TABLE demo_db.t1 options(format='csv', header=false, mode='append');           local
 ---- ------------------ ----------- ------------ --------------- ---------------------------------------------------------------------------------------------------------------------------- --------- ---------------- -------

 1 row in set
```

## 相关语句

[SHOW JOB](./SHOW_JOB.md)

[STOP JOBS](./STOP_JOB.md)