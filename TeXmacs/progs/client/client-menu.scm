
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : client-menu.scm
;; DESCRIPTION : menus for remote TeXmacs services
;; COPYRIGHT   : (C) 2013  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (client client-menu)
  (:use (client client-base) (client client-db) (client client-widgets))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Remote client submenus
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind start-client-menu
  (with l
    (client-accounts)
    (if (null? l) ("Login" (open-remote-login "" "")))
    (if (nnull? l)
      (for (x l)
        (with (server-name pseudo)
          x
          ((eval (string-append "Login as " pseudo "@" server-name))
           (open-remote-login server-name pseudo)
          ) ;
        ) ;with
      ) ;for
      ("Other login" (open-remote-login "" ""))
    ) ;if
    ("New account" (open-remote-account-creator))
  ) ;with
) ;menu-bind

(tm-menu (remote-home-menu server sep?)
  (when (remote-home-directory server)
    ("Home directory" (load-document (remote-home-directory server)))
  ) ;when
  (when (list-chat-rooms server)
    ("Chat rooms" (load-document (list-chat-rooms server)))
  ) ;when
  (when (list-live server)
    ("Live documents" (load-document (list-live server)))
  ) ;when
  (assuming sep? ---)
  (when (list-shared server)
    ("Shared resources" (load-document (list-shared server)))
  ) ;when
  ("Synchronize" (remote-interactive-sync server))
) ;tm-menu

(tm-menu (remote-file-menu server sep?)
 ("Rename" (remote-rename-interactive server))
 ("Remove" (remote-remove-interactive server))
 ("Permissions" (open-permissions-editor server (current-buffer)))
 ("Share" (open-share-document-widget server (current-buffer)))
 (assuming sep? ---)
 ("Download" (simple-interactive-download server))
 ("Synchronize" (simple-interactive-synchronize server))
) ;tm-menu

(tm-menu (remote-dir-menu server sep?)
 ("New remote file" (remote-create-file-interactive server))
 ("New remote directory" (remote-create-dir-interactive server))
 ("Remove" (remote-remove-interactive server))
 ("Permissions" (open-permissions-editor server (current-buffer)))
 ("Share" (open-share-document-widget server (current-buffer)))
 (assuming sep? ---)
 ("Upload" (simple-interactive-upload server))
 ("Download" (simple-interactive-download server))
 ("Synchronize" (simple-interactive-synchronize server))
) ;tm-menu

(tm-menu (remote-chat-menu server)
 ("Permissions" (open-permissions-editor server (current-buffer)))
 ("Invite" (open-share-document-widget server (current-buffer)))
) ;tm-menu

(tm-menu (remote-chat-list-menu server)
 ("New chat room" (chat-room-create-interactive server))
 ("Join chat room" (chat-room-join-interactive server))
) ;tm-menu

(tm-menu (remote-live-menu server)
 ("Permissions" (open-permissions-editor server (current-buffer)))
 ("Share" (open-share-document-widget server (current-buffer)))
) ;tm-menu

(tm-menu (remote-live-list-menu server)
 ("New live document" (live-create-interactive server))
 ("Open live document" (live-open-interactive server))
) ;tm-menu

(tm-menu (remote-mail-menu server)
 ("Incoming messages" (mail-box-open server))
 ("Send message" (open-message-editor server))
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Main remote menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-menu (remote-submenu server)
  (dynamic (remote-home-menu server #f))
  ---
  (if (and (remote-file-name (current-buffer))
        (not (remote-home-directory? (current-buffer)))
      ) ;and
    (group "Remote file")
    (dynamic (remote-file-menu server #f))
  ) ;if
  (if (and (remote-file-name (current-buffer))
        (remote-home-directory? (current-buffer))
      ) ;and
    (group "Remote directory")
    (dynamic (remote-dir-menu server #f))
  ) ;if
  (if (and (chat-room-url? (current-buffer)) (not (mail-box-url? (current-buffer))))
    (group "Chat room")
    (dynamic (remote-chat-menu server))
  ) ;if
  (if (chat-rooms-url? (current-buffer))
    (group "Chat rooms")
    (dynamic (remote-chat-list-menu server))
  ) ;if
  (if (live-url? (current-buffer))
    (group "Live document")
    (dynamic (remote-live-menu server))
  ) ;if
  (if (live-list-url? (current-buffer))
    (group "Live documents")
    (dynamic (remote-live-list-menu server))
  ) ;if
  ---
  (dynamic (remote-mail-menu server))
  ---
  ("Logout" (client-logout server))
) ;tm-menu

(menu-bind client-menu
  (invisible (client-active-servers))
  (with l
    (client-active-servers)
    (assuming (null? l) (link start-client-menu))
    (assuming (== (length l) 1) (dynamic (remote-submenu (car l))))
    (assuming (> (length l) 1)
      (for (server l)
        (-> (eval (client-find-server-name server)) (dynamic (remote-submenu server)))
      ) ;for
    ) ;assuming
  ) ;with
) ;menu-bind

(menu-bind remote-menu
  (if (and (null? remote-client-list) (not (server-started?)))
    (link start-client-menu)
    ;; ---
    ;; (link start-server-menu)
  ) ;if
  (if (and (null? remote-client-list) (server-started?)) (link server-menu))
  (if (nnull? remote-client-list) (link client-menu))
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Main remote icon menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-menu (remote-subicons server)
  (=> (balloon (icon "tm_cloud.xpm") "Connection with server")
   ("Logout" (client-logout server))
  ) ;=>
  (=> (balloon (icon "tm_cloud_home.xpm") "My resources on the server")
    (dynamic (remote-home-menu server #t))
  ) ;=>
  (if (and (remote-file-name (current-buffer))
        (not (remote-home-directory? (current-buffer)))
      ) ;and
    (=> (balloon (icon "tm_cloud_file.xpm") "Remote file")
      (dynamic (remote-file-menu server #t))
    ) ;=>
  ) ;if
  (if (and (remote-file-name (current-buffer))
        (remote-home-directory? (current-buffer))
      ) ;and
    (=> (balloon (icon "tm_cloud_dir.xpm") "Remote directory")
      (dynamic (remote-dir-menu server #t))
    ) ;=>
  ) ;if
  (if (and (chat-room-url? (current-buffer)) (not (mail-box-url? (current-buffer))))
    (=> (balloon (icon "tm_cloud_file.xpm") "Chat room")
      (dynamic (remote-chat-menu server))
    ) ;=>
  ) ;if
  (if (chat-rooms-url? (current-buffer))
    (=> (balloon (icon "tm_cloud_dir.xpm") "Chat rooms")
      (dynamic (remote-chat-list-menu server))
    ) ;=>
  ) ;if
  (if (live-url? (current-buffer))
    (=> (balloon (icon "tm_cloud_file.xpm") "Live document")
      (dynamic (remote-live-menu server))
    ) ;=>
  ) ;if
  (if (live-list-url? (current-buffer))
    (=> (balloon (icon "tm_cloud_dir.xpm") "Live documents")
      (dynamic (remote-live-list-menu server))
    ) ;=>
  ) ;if
  (=> (balloon (icon "tm_cloud_mail.xpm") "Messages")
    (dynamic (remote-mail-menu server))
  ) ;=>
) ;tm-menu

(menu-bind remote-icons
  (invisible (client-active-servers))
  (assuming (and (null? remote-client-list) (not (server-started?)))
    (=> (balloon (icon "tm_cloud.xpm") "Connect with server")
      (link start-client-menu)
    ) ;=>
  ) ;assuming
  (assuming (and (null? remote-client-list) (server-started?))
    (=> (balloon (icon "tm_cloud.xpm") "Server menu") (link server-menu))
  ) ;assuming
  (assuming (and (nnull? remote-client-list) (nnull? (client-active-servers)))
    (dynamic (remote-subicons (car (client-active-servers))))
  ) ;assuming
) ;menu-bind
