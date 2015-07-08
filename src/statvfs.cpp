// Copyright 2012 Mark Cavage <mcavage@gmail.com> All rights reserved.

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/statvfs.h>

#include <nan.h>
#include <node.h>
#include <v8.h>

using namespace v8;


///--- Node definitions

static Persistent<String> bsize_sym;
static Persistent<String> frsize_sym;
static Persistent<String> blocks_sym;
static Persistent<String> bfree_sym;
static Persistent<String> bavail_sym;
static Persistent<String> files_sym;
static Persistent<String> ffree_sym;
static Persistent<String> favail_sym;
static Persistent<String> fsid_sym;
static Persistent<String> flag_sym;
static Persistent<String> namemax_sym;


///--- V8 Nonsense

#define RETURN_EXCEPTION(MSG)										   \
	return NanThrowError(v8::Exception::Error(NanNew<String>(MSG)))

#define RETURN_ARGS_EXCEPTION(MSG)									  \
	return NanThrowError(v8::Exception::TypeError(NanNew<String>(MSG)))

#define REQUIRE_ARGS(ARGS)				  \
	if (ARGS.Length() == 0)					\
		RETURN_ARGS_EXCEPTION("missing arguments");

#define REQUIRE_STRING_ARG(ARGS, I, VAR)				  \
	REQUIRE_ARGS(ARGS);										   \
	if (ARGS.Length() <= (I) || !ARGS[I]->IsString())		\
		RETURN_ARGS_EXCEPTION("argument " #I " must be a String"); \
	v8::String::Utf8Value VAR(ARGS[I]->ToString());

#define REQUIRE_FUNCTION_ARG(ARGS, I, VAR)							  \
	REQUIRE_ARGS(ARGS);						\
	if (ARGS.Length() <= (I) || !ARGS[I]->IsFunction())				   \
		RETURN_EXCEPTION("argument " #I " must be a Function");	\
	v8::Local<v8::Function> VAR = v8::Local<v8::Function>::Cast(ARGS[I]);



///--- libuv "baton"

class my_baton_t {
public:
	my_baton_t() {
		path = NULL;
		rc = 0;
		_errno = 0;
		memset(&buf, 0, sizeof (struct statvfs));
	};

	virtual ~my_baton_t() {
		if (path != NULL)
			free(path);
		path = NULL;
	};

	// Inputs
	char *path;
	NanCallback *callback;

	// Output
	int rc;
	int _errno;
	struct statvfs buf;

private:
	my_baton_t(const my_baton_t &);
	my_baton_t &operator=(const my_baton_t &);
};


Local<Object> build_stats_object(struct statvfs &s) {
	NanEscapableScope();

	Local<Object> stats = NanNew<Object>();

	stats->Set(NanNew<String>(bsize_sym), NanNew<Number>(s.f_bsize));
	stats->Set(NanNew<String>(frsize_sym), NanNew<Number>(s.f_frsize));
	stats->Set(NanNew<String>(blocks_sym), NanNew<Number>(s.f_blocks));
	stats->Set(NanNew<String>(bfree_sym), NanNew<Number>(s.f_bfree));
	stats->Set(NanNew<String>(bavail_sym), NanNew<Number>(s.f_bavail));
	stats->Set(NanNew<String>(files_sym), NanNew<Number>(s.f_files));
	stats->Set(NanNew<String>(ffree_sym), NanNew<Number>(s.f_ffree));
	stats->Set(NanNew<String>(favail_sym), NanNew<Number>(s.f_favail));
	stats->Set(NanNew<String>(fsid_sym), NanNew<Number>(s.f_fsid));
	stats->Set(NanNew<String>(flag_sym), NanNew<Number>(s.f_flag));
	stats->Set(NanNew<String>(namemax_sym), NanNew<Number>(s.f_namemax));

	return NanEscapeScope(stats);
}



///--- Async APIs

void _statvfs (uv_work_t *req) {
	my_baton_t *baton = (my_baton_t *)req->data;

	baton->rc = statvfs(baton->path, &(baton->buf));
	if (baton->rc != 0)
		baton->_errno = errno;
}


void _statvfs_after (uv_work_t *req, int ignore_me_status_in_0_dot_10) {
	NanScope();


	int argc = 1;
	Handle<Value> argv[2];

	my_baton_t *baton = (my_baton_t *)req->data;
	if (baton->rc < 0) {
		argv[0] = node::ErrnoException(baton->_errno, "statvfs");
	} else {
		argc = 2;
		argv[0] = NanNull();
		argv[1] = build_stats_object(baton->buf);
	}

	TryCatch try_catch;
	baton->callback->Call(argc, argv);
	if (try_catch.HasCaught())
		node::FatalException(try_catch);

	delete baton->callback;
	delete baton;
	delete req;
}

NAN_METHOD(_statvfs_entry) {
	NanScope();

	REQUIRE_STRING_ARG(args, 0, name);
	REQUIRE_FUNCTION_ARG(args, 1, callback);

	my_baton_t *baton = new my_baton_t;
	if (!baton)
		RETURN_EXCEPTION("out of memory");
	baton->callback = new NanCallback(callback);
	baton->path = strdup(*name);
	if (!baton->path) {
		delete baton;
		RETURN_EXCEPTION("out of memory");
	}

	uv_work_t *req = new uv_work_t;
	if (req == NULL) {
		delete baton;
		RETURN_EXCEPTION("out of memory");
	}

	req->data = baton;
	uv_queue_work(uv_default_loop(), req, _statvfs,
		(uv_after_work_cb)_statvfs_after);

	NanReturnUndefined();
}



///--- Actually expose this to the outside world
void init(Handle<Object> target) {
	NanAssignPersistent(bsize_sym, NanNew<String>("bsize"));
	NanAssignPersistent(frsize_sym, NanNew<String>("frsize"));
	NanAssignPersistent(blocks_sym, NanNew<String>("blocks"));
	NanAssignPersistent(bfree_sym, NanNew<String>("bfree"));
	NanAssignPersistent(bavail_sym, NanNew<String>("bavail"));
	NanAssignPersistent(files_sym, NanNew<String>("files"));
	NanAssignPersistent(ffree_sym, NanNew<String>("ffree"));
	NanAssignPersistent(favail_sym, NanNew<String>("favail"));
	NanAssignPersistent(fsid_sym, NanNew<String>("fsid"));
	NanAssignPersistent(flag_sym, NanNew<String>("flag"));
	NanAssignPersistent(namemax_sym, NanNew<String>("namemax"));

	NODE_SET_METHOD(target, "statvfs", _statvfs_entry);
}

NODE_MODULE(statvfs, init)
