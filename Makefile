all: 
	gcc -g -O0 -Wall myfs.c implementation.c `pkg-config fuse --cflags --libs` -o myfs
run:
	gdb --args ./myfs --backupfile=test.myfs /home/rgarcia117/fuse-mnt/ -f
unmount:
	fusermount -u ~/fuse-mnt
clean:
	rm -rf myfs Report.pdf
push:
	@read -p "Enter commit message: " msg; \
	git status; \
	git add -A; \
	echo "\033[33mStaging changes...\033[0m"; \
	git status; \
	if git diff --cached --exit-code > /dev/null; then \
		echo "No changes to commit."; \
	else \
		git commit -m "$$msg"; \
		echo -e "\033[34mPulling latest changes with rebase...\033[0m"; \
		git pull --rebase origin main; \
		git push origin main; \
	fi
pull:
	@echo -e "\033[34mPulling latest changes with rebase...\033[0m"; \
	git pull origin main --rebase
commit:
	@read -p "Enter commit message: " msg; \
	git status; \
	git add -A; \
	echo "\033[33mStaging changes...\033[0m"; \
	git status; \
	if git diff --cached --exit-code > /dev/null; then \
		echo "No changes to commit."; \
	else \
		git commit -m "$$msg"; \
	fi
pdf:
	pandoc README.md -o Report.pdf
tar:
	tar -czvf assignment3.tgz .

