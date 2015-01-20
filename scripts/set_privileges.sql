
GRANT SELECT ON `nuclei`.* TO 'guest'@'localhost';
GRANT SELECT, INSERT ON `nuclei`.`training_sets` TO 'guest'@'localhost';
GRANT INSERT ON `nuclei`.`training_objs` TO 'guest'@'localhost';
GRANT INSERT ON `nuclei`.`classes` TO 'guest'@'localhost';
GRANT SELECT ON `boundary`.`boundaries` TO 'guest'@'localhost'; 
GRANT INSERT ON `nuclei`.`logs` TO 'logger'@'localhost';

