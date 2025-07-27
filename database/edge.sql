/* This is an example on how the database can be configure */
/* There is a table for each end device */
DROP TABLE IF EXISTS tags;
CREATE TABLE tags (
  id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  tag_name TEXT NOT NULL,
  UNIQUE(tag_name)
);
DROP TABLE IF EXISTS tt001;
CREATE TABLE tt001 (
  id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  tag_id TEXT NOT NULL,
  val NUMERIC NOT NULL,
  time_stamp DATETIME DEFAULT current_timestamp,
  FOREIGN KEY(tag_id) REFERENCES tags(id)
);
DROP TABLE IF EXISTS pt001;
CREATE TABLE pt001 (
  id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  tag_id TEXT NOT NULL,
  val NUMERIC NOT NULL,
  time_stamp DATETIME DEFAULT current_timestamp,
  FOREIGN KEY(tag_id) REFERENCES tags(id)
);
DROP TABLE IF EXISTS ft001;
CREATE TABLE ft001 (
  id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  tag_id TEXT NOT NULL,
  val NUMERIC NOT NULL,
  time_stamp DATETIME DEFAULT current_timestamp,
  FOREIGN KEY(tag_id) REFERENCES tags(id)
);
DROP TABLE IF EXISTS lt001;
CREATE TABLE lt001 (
  id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  tag_id TEXT NOT NULL,
  val NUMERIC NOT NULL,
  time_stamp DATETIME DEFAULT current_timestamp,
  FOREIGN KEY(tag_id) REFERENCES tags(id)
);

INSERT INTO tags (tag_name) VALUES ("tt001");
INSERT INTO tags (tag_name) VALUES ("pt001");
INSERT INTO tags (tag_name) VALUES ("ft001");
INSERT INTO tags (tag_name) VALUES ("lt001");
