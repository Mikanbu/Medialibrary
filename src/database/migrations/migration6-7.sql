"DROP TABLE " + GenreTable::Name + ";",

"CREATE TABLE " + GenreTable::Name + "("
"id_genre INTEGER PRIMARY KEY AUTOINCREMENT,"
"name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
"nb_tracks INTEGER NOT NULL DEFAULT 0);",
