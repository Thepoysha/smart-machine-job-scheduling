CREATE DATABASE IF NOT EXISTS manufacturing_scheduler;
USE manufacturing_scheduler;

CREATE TABLE IF NOT EXISTS machines (
    machine_id INT AUTO_INCREMENT PRIMARY KEY,
    machine_name VARCHAR(100) NOT NULL UNIQUE,
    available_at INT NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS jobs (
    job_id INT AUTO_INCREMENT PRIMARY KEY,
    job_name VARCHAR(100) NOT NULL,
    processing_time INT NOT NULL,
    priority INT NOT NULL DEFAULT 3,
    status ENUM('PENDING','SCHEDULED') NOT NULL DEFAULT 'PENDING',
    machine_id INT NULL,
    start_time INT NULL,
    finish_time INT NULL,
    waiting_time INT NULL,
    algorithm VARCHAR(20) NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (machine_id) REFERENCES machines(machine_id)
);

INSERT IGNORE INTO machines (machine_name, available_at) VALUES
('Machine A',0), ('Machine B',0), ('Machine C',0);
