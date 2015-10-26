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

static Nan::Persistent<String> bsize_sym;
static Nan::Persistent<String> frsize_sym;
static Nan::Persistent<String> blocks_sym;
static Nan::Persistent<String> bfree_sym;
static Nan::Persistent<String> bavail_sym;
static Nan::Persistent<String> files_sym;
static Nan::Persistent<String> ffree_sym;
static Nan::Persistent<String> favail_sym;
static Nan::Persistent<String> fsid_sym;
static Nan::Persistent<String> flag_sym;
static Nan::Persistent<String> namemax_sym;


///--- V8 Nonsense

#define RETURN_EXCEPTION(MSG)										   \
	return Nan::ThrowError(v8::Exception::Error(Nan::New<String>(MSG).ToLocalChecked()))

#define RETURN_ARGS_EXCEPTION(MSG)									  \
	return Nan::ThrowError(v8::Exception::TypeError(Nan::New<String>(MSG).ToLocalChecked()))

#define REQUIRE_ARGS(ARGS)				  \
	if (ARGS.Length() == 0)					\
		RETURN_ARGS_EXCEPTION("missing arguments");

#define REQUIRE_STRING_ARG(ARGS, I, VAR)				  \
	REQUIRE_ARGS(ARGS);										   \
	if (ARGS.Length() <= (I) || !ARGS[I]->IsString())		\
		RETURN_ARGS_EXCEPTION("argument " #I " must be a String"); \
	Nan::Utf8String VAR(ARGS[I]->ToString());

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
	Nan::Callback *callback;

	// Output
	int rc;
	int _errno;
	struct statvfs buf;

private:
	my_baton_t(const my_baton_t &);
	my_baton_t &operator=(const my_baton_t &);
};


Local<Object> build_stats_object(struct statvfs &s) {
	Nan::EscapableHandleScope scope;

	Local<Object> stats = Nan::New<Object>();

	Nan::Set(stats, Nan::New<String>(bsize_sym), Nan::New<Number>(s.f_bsize));
	Nan::Set(stats, Nan::New<String>(frsize_sym), Nan::New<Number>(s.f_frsize));
	Nan::Set(stats, Nan::New<String>(blocks_sym), Nan::New<Number>(s.f_blocks));
	Nan::Set(stats, Nan::New<String>(bfree_sym), Nan::New<Number>(s.f_bfree));
	Nan::Set(stats, Nan::New<String>(bavail_sym), Nan::New<Number>(s.f_bavail));
	Nan::Set(stats, Nan::New<String>(files_sym), Nan::New<Number>(s.f_files));
	Nan::Set(stats, Nan::New<String>(ffree_sym), Nan::New<Number>(s.f_ffree));
	Nan::Set(stats, Nan::New<String>(favail_sym), Nan::New<Number>(s.f_favail));
	Nan::Set(stats, Nan::New<String>(fsid_sym), Nan::New<Number>(s.f_fsid));
	Nan::Set(stats, Nan::New<String>(flag_sym), Nan::New<Number>(s.f_flag));
	Nan::Set(stats, Nan::New<String>(namemax_sym), Nan::New<Number>(s.f_namemax));

	return scope.Escape(stats);
}



///--- Async APIs

void _statvfs (uv_work_t *req) {
	my_baton_t *baton = (my_baton_t *)req->data;

	baton->rc = statvfs(baton->path, &(baton->buf));
	if (baton->rc != 0)
		baton->_errno = errno;
}


void _statvfs_after (uv_work_t *req, int ignore_me_status_in_0_dot_10) {
	Nan::HandleScope scope;

	int argc = 1;
	v8::Local<Value> argv[2];

	my_baton_t *baton = (my_baton_t *)req->data;
	if (baton->rc < 0) {
		argv[0] = Nan::ErrnoException(baton->_errno,
		    "statvfs", "statvfs() failed", baton->path);
	} else {
		argc = 2;
		argv[0] = Nan::Null();
		argv[1] = build_stats_object(baton->buf);
	}

	Nan::TryCatch try_catch;
	Nan::Call(*baton->callback, argc, argv);
	if (try_catch.HasCaught())
		Nan::FatalException(try_catch);

	delete baton->callback;
	delete baton;
	delete req;
}

NAN_METHOD(_statvfs_entry) {

	REQUIRE_STRING_ARG(info, 0, name);
	REQUIRE_FUNCTION_ARG(info, 1, callback);

	my_baton_t *baton = new my_baton_t;
	if (!baton)
		RETURN_EXCEPTION("out of memory");
	baton->callback = new Nan::Callback(callback);
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
}



///--- Actually expose this to the outside world
void init(v8::Local<Object> target) {
	bsize_sym.Reset(Nan::New<String>("bsize").ToLocalChecked());
	frsize_sym.Reset(Nan::New<String>("frsize").ToLocalChecked());
	blocks_sym.Reset(Nan::New<String>("blocks").ToLocalChecked());
	bfree_sym.Reset(Nan::New<String>("bfree").ToLocalChecked());
	bavail_sym.Reset(Nan::New<String>("bavail").ToLocalChecked());
	files_sym.Reset(Nan::New<String>("files").ToLocalChecked());
	ffree_sym.Reset(Nan::New<String>("ffree").ToLocalChecked());
	favail_sym.Reset(Nan::New<String>("favail").ToLocalChecked());
	fsid_sym.Reset(Nan::New<String>("fsid").ToLocalChecked());
	flag_sym.Reset(Nan::New<String>("flag").ToLocalChecked());
	namemax_sym.Reset(Nan::New<String>("namemax").ToLocalChecked());

	Nan::SetMethod(target, "statvfs", _statvfs_entry);
}

NODE_MODULE(statvfs, init)
