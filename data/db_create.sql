CREATE TABLE alias (name TEXT, alias TEXT);
CREATE TABLE config (id INTEGER PRIMARY KEY, db_version INTEGER, last_update INTEGER, has_new INTEGER, last_check INTEGER);
INSERT INTO config (last_update, has_new, last_check) VALUES (0, 0, 0, 0);
CREATE TABLE universe (name TEXT, generic_name TEXT, is_desktop INTEGER, category TEXT, arch TEXT, version TEXT, exec TEXT, rir INTEGER, priority TEXT, install TEXT, license TEXT, homepage TEXT, repo TEXT, size INTEGER, sha TEXT, build_date INTEGER, packager TEXT, uri TEXT, description TEXT,installed INTEGER DEFAULT 0, can_update INTEGER DEFAULT 0, data_count INTEGER, PRIMARY KEY(name));
CREATE TABLE universe_data (name TEXT, version TEXT, data_name TEXT, data_format TEXT, data_size INTEGER, data_install_size INTEGER, data_depend TEXT, data_bdepend TEXT, data_recommended TEXT, data_conflict TEXT);
CREATE TABLE universe_testing (name TEXT, generic_name TEXT, is_desktop INTEGER, category TEXT, arch TEXT, version TEXT, exec TEXT, rir INTEGER, priority TEXT, install TEXT, license TEXT, homepage TEXT, repo TEXT, size INTEGER, sha TEXT, build_date INTEGER, packager TEXT, uri TEXT, description TEXT,installed INTEGER DEFAULT 0, can_update INTEGER DEFAULT 0, data_count INTEGER, PRIMARY KEY(name));
CREATE TABLE universe_testing_data (name TEXT, version TEXT, data_name TEXT, data_format TEXT, data_size INTEGER, data_install_size INTEGER, data_depend TEXT, data_bdepend TEXT, data_recommended TEXT, data_conflict TEXT);
CREATE TABLE universe_history (name TEXT, generic_name TEXT, is_desktop INTEGER, category TEXT, arch TEXT, version TEXT, exec TEXT, rir INTEGER, priority TEXT, install TEXT, license TEXT, homepage TEXT, repo TEXT, size INTEGER, sha TEXT, build_date INTEGER, packager TEXT, uri TEXT, description TEXT,installed INTEGER DEFAULT 0, data_count INTEGER, PRIMARY KEY(name,version));
CREATE TABLE universe_history_data (name TEXT, version TEXT, data_name TEXT, data_format TEXT, data_size INTEGER, data_install_size INTEGER, data_depend TEXT, data_bdepend TEXT, data_recommended TEXT, data_conflict TEXT);
CREATE TABLE universe_language (name TEXT, version TEXT, language TEXT,  generic_name TEXT, description TEXT );
CREATE TABLE world (name TEXT, generic_name TEXT, is_desktop INTEGER, category TEXT, arch TEXT, version TEXT, exec TEXT, rir INTEGER, priority TEXT, install TEXT, license TEXT, homepage TEXT, repo TEXT, size INTEGER, sha TEXT, build_date INTEGER, packager TEXT, uri TEXT, description TEXT, can_update INTEGER DEFAULT 0 , data_count INTEGER, install_time INTEGER, PRIMARY KEY(name));
CREATE TABLE world_data (name TEXT, version TEXT, data_name TEXT, data_format TEXT, data_size INTEGER, data_install_size INTEGER, data_depend TEXT, data_bdepend TEXT, data_recommended TEXT, data_conflict TEXT);
CREATE TABLE world_file (name TEXT, version TEXT, file TEXT , type TEXT, size INTEGER, perms TEXT, uid INTEGER, gid INTEGER, mtime INTEGER, extra TEXT);
CREATE TABLE world_language (name TEXT, version TEXT, language TEXT,  generic_name TEXT, description TEXT );

CREATE TABLE keywords (name TEXT, language TEXT, kw_name TEXT, kw_generic_name TEXT, kw_fullname TEXT, kw_comment , PRIMARY KEY(name,language) );

CREATE INDEX universe_data_name ON universe_data ( name );
CREATE INDEX universe_testing_data_name ON universe_testing_data ( name );
CREATE INDEX universe_history_name ON universe_history ( name );
CREATE INDEX universe_history_version ON universe_history ( version );
CREATE INDEX universe_history_data_name ON universe_history_data ( name );
CREATE INDEX universe_history_data_version ON universe_history_data ( version );
CREATE INDEX universe_language_name ON universe_language ( name );
CREATE INDEX universe_language_version ON universe_language ( version );
CREATE INDEX universe_language_language ON universe_language ( language );
CREATE INDEX world_data_name ON world_data ( name );
CREATE INDEX world_language_language ON world_language ( language );
CREATE INDEX world_language_name ON world_language ( name );
