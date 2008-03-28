; some useful stuff for editing 65c02 assembly

(setq-default indent-tabs-mode nil)

(add-hook 'asm-mode-hook 'my-asm-mode)

(defun my-asm-mode ()
  (local-set-key ";" 'self-insert-command)
  (local-set-key ":" 'self-insert-command)
  (local-set-key [(control ?;)] 'asm-comment)
  (setq tab-stop-list (delete 8 tab-stop-list)))

(defun my-asm-label ()
  "Insert a label without a colon"
  (interactive)
  (save-excursion
    (beginning-of-line)
    (if (looking-at "[ \t]+\\(\\sw\\|\\s_\\)+$")
	(delete-horizontal-space)))
  (tab-to-tab-stop))
