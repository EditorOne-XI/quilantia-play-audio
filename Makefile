PKG_NAME      	:= Quilantia Play Audio
PKG_VERSION		:= 1.0
CMD_NAME        := qpa

PREFIX      ?= /usr/local
BIN_DIR     := $(PREFIX)/bin
ZSH_DIR     := $(PREFIX)/share/zsh/site-functions
FISH_DIR    := $(PREFIX)/share/fish/vendor_completions.d
BASH_DIR    := $(PREFIX)/share/bash-completion/completions
TMP_DIR     := $(PREFIX)/tmp

CC          := clang
CFLAGS      := -O3 -flto \
               -Wall -Wextra -Wformat -Wformat-security \
               -ffunction-sections -fdata-sections \
               -D_XOPEN_SOURCE=600 \
               -D_POSIX_C_SOURCE=200809L
DFLAGS		:= -DFIFO_PATH="\"$(TMP_DIR)/$(CMD_NAME)_cava_fifo\"" \
			   -DCONF_PATH="\"$(TMP_DIR)/$(CMD_NAME)_cava_config\"" \
			   -DPACKAGE_NAME="\"$(PKG_NAME)\"" \
			   -DPACKAGE_VERSION="\"$(PKG_VERSION)\""
LDFLAGS     := -lncursesw -ltinfo -Wl,--gc-sections -s
HEADERS     := src/stringcustom.h src/audio_metadata.h

.PHONY: all help install uninstall dep dep-sox udep udep-sox ualldep compile clean

all: help

_completions:
	@mkdir -p $(ZSH_DIR) $(FISH_DIR) $(BASH_DIR)
	@echo "Generating completions for $(CMD_NAME)..."
	@printf "#compdef $(CMD_NAME)\n_arguments '(-h --help)'{-h,--help}'[Verbose this help and exit]' '--status[Verbose qpa install status and exit]' '(-v --version)'{-v,--version}'[Verbose qpa version and exit]' '(-m --mouse)'{-m,--mouse}'[Enable mouse/screen touch]' '--use-cava[Use cava for loop mode]' '(-g --grep)'{-g,--grep}'[Grep command to filter files]:grep syntax:' '(-r --recursive)'{-r,--recursive}'[Read files recursively]::depth (N):' '(-s --sort)'{-s,--sort}'[Sort alphabetically]:sort order:((A\:ascending D\:descending))' '*:directory:_files -/'" > $(ZSH_DIR)/_$(CMD_NAME)
	@printf "complete -c $(CMD_NAME) -f\ncomplete -c $(CMD_NAME) -s h -l help -d 'Verbose this help and exit'\ncomplete -c $(CMD_NAME) -l status -d 'Verbose qpa install status and exit'\ncomplete -c $(CMD_NAME) -s v -l version -d 'Verbose qpa version and exit'\ncomplete -c $(CMD_NAME) -s m -l mouse -d 'Enable mouse/screen touch'\ncomplete -c $(CMD_NAME) -l use-cava -d 'Use cava for loop mode'\ncomplete -c $(CMD_NAME) -s g -l grep -d 'Grep command to filter files' -r\ncomplete -c $(CMD_NAME) -s r -l recursive -d 'Read files recursively' -a '0 1 2 3 4 5'\ncomplete -c $(CMD_NAME) -s s -l sort -d 'Sort order' -xa 'A D'\ncomplete -c $(CMD_NAME) -a '(__fish_complete_directories)'" > $(FISH_DIR)/$(CMD_NAME).fish
	@printf "_$(CMD_NAME)_completions() {\n  local cur prev\n  COMPREPLY=()\n  cur=\"\$${COMP_WORDS[COMP_CWORD]}\"\n  prev=\"\$${COMP_WORDS[COMP_CWORD-1]}\"\n  local opts=(\"-h\" \"--help\" \"--status\" \"-v\" \"--version\" \"-m\" \"--mouse\" \"--use-cava\" \"-g\" \"--grep\" \"-r\" \"--recursive\" \"-s\" \"--sort\")\n  case \"\$$prev\" in\n    -s|--sort) mapfile -t COMPREPLY < <(compgen -W \"A D\" -- \"\$$cur\"); return 0 ;;\n    -g|--grep) return 0 ;;\n    -r|--recursive) mapfile -t COMPREPLY < <(compgen -W \"0 1 2 3 4 5\" -- \"\$$cur\"); return 0 ;;\n  esac\n  if [[ \$$cur == -* ]] ; then\n    mapfile -t COMPREPLY < <(compgen -W \"\$${opts[*]}\" -- \"\$$cur\"); return 0;\n  fi\n  mapfile -t COMPREPLY < <(compgen -d -- \"\$$cur\")\n}\ncomplete -F _$(CMD_NAME)_completions $(CMD_NAME)" > $(BASH_DIR)/$(CMD_NAME)

$(CMD_NAME): src/qpa.c $(HEADERS)
	$(CC) $(CFLAGS) $(DFLAGS) $< -o $@ $(LDFLAGS)
	install -Dm755 $(CMD_NAME) $(BIN_DIR)/$(CMD_NAME)

install: $(CMD_NAME) _completions
	@pkg install play-audio -y || true
	@echo "$(PKG_NAME) has been installed!"

uninstall:
	rm -f $(BIN_DIR)/$(CMD_NAME) $(ZSH_DIR)/_$(CMD_NAME) $(FISH_DIR)/$(CMD_NAME).fish $(BASH_DIR)/$(CMD_NAME)
	rm -f $(CMD_NAME)
	@echo "$(PKG_NAME) uninstalled."

dep: install
	@pkg install ffmpeg pv cava -y || true

dep-sox: install
	@pkg install sox pv cava -y || true

udep: uninstall
	@pkg uninstall ffmpeg pv cava -y || true

udep-sox: uninstall
	@pkg uninstall sox pv cava -y || true

ualldep: uninstall
	@pkg uninstall play-audio ffmpeg sox pv cava -y || true

compile: $(CMD_NAME)
	@echo "Compiled $(CMD_NAME) successfully."

clean:
	rm -f $(CMD_NAME)

help:
	@echo "$(PKG_NAME) Makefile"
	@echo ""
	@echo "Build Options:"
	@printf "%-24s%s\n" "    make install" "Install $(CMD_NAME) (basic)"
	@printf "%-24s%s\n" "    make dep" "Install $(CMD_NAME) + (ffmpeg, pv, cava)"
	@printf "%-24s%s\n" "    make dep-sox" "Install $(CMD_NAME) + (sox, pv, cava)"
	@printf "%-24s%s\n" "    make uninstall" "Uninstall $(CMD_NAME)"
	@echo ""
	@echo "Others:"
	@printf "%-24s%s\n" "    make compile" "Only compile $(CMD_NAME) binary"
	@printf "%-24s%s\n" "    make ualldep" "Uninstall $(CMD_NAME) and all dependencies"
