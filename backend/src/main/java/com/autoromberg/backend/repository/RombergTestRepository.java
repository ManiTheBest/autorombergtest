package com.autoromberg.backend.repository;

import com.autoromberg.backend.domain.RombergTest;
import java.util.List;
import org.springframework.data.jpa.repository.JpaRepository;

public interface RombergTestRepository extends JpaRepository<RombergTest, Long> {

	List<RombergTest> findAllByOrderByCreatedAtDesc();

	boolean existsBySessionName(String sessionName);

	List<RombergTest> findBySource(String source);
}
