/*
    This file is part of Git Native Prompt.
    Git Native Prompt is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Git Native Prompt is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Git Native Prompt.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <git2.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef DEBUG_TO_STDOUT
#define log(...) do { printf(__VA_ARGS__); } while (0)
#else
# define log(...)
#endif


const char STASH_GLYPH[] = {0xF0, 0x9F, 0x97, 0x90, 0};
void init();
void shutdown();

int status_cb(const char *, unsigned int, void *);
int stash_cb(size_t, const char*, const git_oid *, void *);
typedef struct {
    int modified;
    int staged;
    int stashed;
} status_data;


int main() {
    init();
    git_repository *repo = NULL;
    git_buf root = {0};
    int error = git_repository_discover(&root, ".", 0, NULL);

    if (error == GIT_ENOTFOUND) {
        // not inside a git repo
        return 0;
    }
    char* path = root.ptr;

    error = git_repository_open(&repo, path);
    git_buf_free(&root);
    if (error < 0) {
        const git_error *e = giterr_last();
        log("Error %d/%d: %s\n", error, e->klass, e->message);
        exit(error);
    }

    status_data d = {0};
    error = git_status_foreach(repo, status_cb, &d);
    if (error < 0) {
        const git_error *e = giterr_last();
        log("Error %d/%d: %s\n", error, e->klass, e->message);
        exit(error);
    }
    char extras[20] = "";
    int size_left = 20; // this will be reduced with each added info
    char *curr_extras = extras;
    if (d.staged) {
        int taken = snprintf(curr_extras, size_left, "\x1b[32m%dS\x1b[0m", d.staged);
        curr_extras += taken;
        size_left -= taken;
    }
    if (d.modified) {
        if (curr_extras != extras) {
            int taken = snprintf(curr_extras, size_left, "|");
            curr_extras += taken;
            size_left -= taken;
        }
        int taken = snprintf(curr_extras, size_left, "\x1b[31m%dM\x1b[0m", d.modified);
        curr_extras += taken;
        size_left -= taken;
    }

    error = git_stash_foreach(repo, stash_cb, &d);
    if (error < 0) {
        const git_error *e = giterr_last();
        log("Error %d/%d: %s\n", error, e->klass, e->message);
        exit(error);
    }

    if (d.stashed) {
        if (curr_extras != extras) {
            int taken = snprintf(curr_extras, size_left, "|");
            curr_extras += taken;
            size_left -= taken;
        }
        int taken = snprintf(curr_extras, size_left, "\x1b[34m%d#\x1b[0m", d.stashed);
        curr_extras += taken;
        size_left -= taken;
    }
    git_reference *ref = NULL;
    const char* buf = "DETACHED";
    error = git_repository_head(&ref, repo);
    if (error == GIT_EUNBORNBRANCH) {
        if (git_repository_is_empty(repo)) {
            buf = "New repo";
        }
    } else if (error < 0) {
        log("error: %d\n", error);

    } else {
        error = git_reference_dwim(&ref, repo, "HEAD");
        if (error) {
            log("git_ref_dwim: %i\n", error);
        }

        // git_branch_name returns -1 when the branch is "neither local or remote branch"
        // which means it's probably DETACHED
        error = git_branch_name(&buf, ref);
        log("git_branch_name: %i\n", error);
    }
    //print branch name
    printf("\x1b[33m[%s\x1b[0m", buf);
    if (extras[0]) {
        // print info about the status
        printf(" %s", extras);
    }
    printf(" ");

    shutdown();
}

#if LIBGIT2_VER_MAJOR != 0
#error LIBGIT2_VER_MAJOR is not 0
#endif

void init() {
#if LIBGIT2_VER_MINOR > 20
    git_libgit2_init();
#else
    git_threads_init();
#endif

}

void shutdown() {
#if LIBGIT2_VER_MINOR > 20
    git_libgit2_shutdown();
#else
    git_threads_shutdown();
#endif
}

int status_cb(const char *path, unsigned int status_flags, void *payload)
{
        status_data *d = (status_data*)payload;
        // any of the following flags means the file is staged for commit
        int staged_flags = GIT_STATUS_INDEX_NEW | GIT_STATUS_INDEX_MODIFIED |
            GIT_STATUS_INDEX_DELETED | GIT_STATUS_INDEX_RENAMED |
            GIT_STATUS_INDEX_TYPECHANGE;

        if (status_flags & staged_flags) {
            d->staged++;
        }

        int modified_flags = GIT_STATUS_WT_MODIFIED |
            GIT_STATUS_WT_DELETED | GIT_STATUS_WT_RENAMED |
            GIT_STATUS_WT_TYPECHANGE;

        if (status_flags & modified_flags) {
            d->modified++;
        }
        return 0;
}

int stash_cb( size_t index, const char* message,
              const git_oid *stash_id, void *payload) {
    status_data *d = (status_data*)payload;
    d->stashed++;
    return 0;

}
